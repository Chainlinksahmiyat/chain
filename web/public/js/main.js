document.addEventListener('DOMContentLoaded', function() {
    // DOM Elements
    const navLinks = document.querySelectorAll('nav a');
    const sections = document.querySelectorAll('.section');
    
    // Modals
    const loginModal = document.getElementById('login-modal');
    const registerModal = document.getElementById('register-modal');
    const loginButton = document.getElementById('login-button');
    const registerButton = document.getElementById('register-button');
    const logoutButton = document.getElementById('logout-button');
    const closeButtons = document.querySelectorAll('.close');
    
    // Forms
    const loginForm = document.getElementById('login-form');
    const registerForm = document.getElementById('register-form');
    const uploadForm = document.getElementById('upload-form');
    const transferForm = document.querySelector('.transfer-form');
    
    // Dashboard elements
    const walletBalance = document.getElementById('wallet-balance');
    const walletAddress = document.getElementById('wallet-address');
    const memoriesList = document.getElementById('memories-list');
    const noMemoriesMessage = document.getElementById('no-memories-message');
    const transactionList = document.getElementById('transaction-list');
    const noTransactionsMessage = document.getElementById('no-transactions-message');
    const mineButton = document.getElementById('mine-button');
    
    // Upload elements
    const memoryFile = document.getElementById('memory-file');
    const previewContainer = document.getElementById('preview-container');
    
    // Blockchain explorer elements
    const blockCount = document.getElementById('block-count');
    const transactionCount = document.getElementById('transaction-count');
    const memoryCount = document.getElementById('memory-count');
    const blocksContainer = document.getElementById('blocks-container');
    const refreshBlockchain = document.getElementById('refresh-blockchain');
    
    // Session state
    let isLoggedIn = false;
    let currentUser = {
        address: '',
        token: ''
    };
    
    // Check if user is already logged in (from localStorage)
    function checkAuth() {
        const savedToken = localStorage.getItem('token');
        const savedAddress = localStorage.getItem('address');
        
        if (savedToken && savedAddress) {
            // Validate token with server
            fetch('/api/balance', {
                headers: {
                    'Authorization': savedToken
                }
            })
            .then(response => {
                if (response.ok) {
                    // Token is valid
                    currentUser.token = savedToken;
                    currentUser.address = savedAddress;
                    updateAuthUI(true);
                    return response.json();
                } else {
                    // Token is invalid
                    localStorage.removeItem('token');
                    localStorage.removeItem('address');
                    updateAuthUI(false);
                    throw new Error('Invalid token');
                }
            })
            .then(data => {
                // Update wallet balance
                walletBalance.textContent = data.balance.toFixed(2);
                walletAddress.textContent = data.address;
                
                // Load user memories and transactions
                loadUserMemories();
                loadUserTransactions();
            })
            .catch(error => {
                console.error('Authentication error:', error);
            });
        } else {
            updateAuthUI(false);
        }
    }
    
    // Update UI based on authentication state
    function updateAuthUI(isAuthenticated) {
        isLoggedIn = isAuthenticated;
        
        if (isAuthenticated) {
            loginButton.style.display = 'none';
            registerButton.style.display = 'none';
            logoutButton.style.display = 'inline-block';
            
            // Enable protected sections
            document.getElementById('nav-dashboard').classList.remove('disabled');
            document.getElementById('nav-upload').classList.remove('disabled');
            
            // If on home page, navigate to dashboard
            if (document.getElementById('home-section').classList.contains('active')) {
                navigateToSection('dashboard-section');
                document.getElementById('nav-dashboard').classList.add('active');
                document.getElementById('nav-home').classList.remove('active');
            }
        } else {
            loginButton.style.display = 'inline-block';
            registerButton.style.display = 'inline-block';
            logoutButton.style.display = 'none';
            
            // Disable protected sections
            document.getElementById('nav-dashboard').classList.add('disabled');
            document.getElementById('nav-upload').classList.add('disabled');
            
            // If on a protected section, navigate to home
            const currentSection = document.querySelector('.section.active');
            if (currentSection && (currentSection.id === 'dashboard-section' || currentSection.id === 'upload-section')) {
                navigateToSection('home-section');
                document.getElementById('nav-home').classList.add('active');
                document.querySelectorAll('nav a:not(#nav-home)').forEach(link => {
                    link.classList.remove('active');
                });
            }
            
            // Clear user data
            walletBalance.textContent = '0.00';
            walletAddress.textContent = 'Not logged in';
            memoriesList.innerHTML = '<p id="no-memories-message">No memories uploaded yet.</p>';
            transactionList.innerHTML = '<p id="no-transactions-message">No transactions yet.</p>';
        }
    }
    
    // Navigation
    function navigateToSection(sectionId) {
        // Hide all sections
        sections.forEach(section => {
            section.classList.remove('active');
        });
        
        // Show target section
        document.getElementById(sectionId).classList.add('active');
    }
    
    // Load user memories
    function loadUserMemories() {
        fetch('/api/memories', {
            headers: {
                'Authorization': currentUser.token
            }
        })
        .then(response => response.json())
        .then(data => {
            if (data.memories && data.memories.length > 0) {
                noMemoriesMessage.style.display = 'none';
                memoriesList.innerHTML = '';
                
                data.memories.forEach(memory => {
                    const memoryElement = document.createElement('div');
                    memoryElement.className = 'memory-item';
                    
                    let icon = '📄';
                    if (memory.type === 'IMAGE') icon = '🖼️';
                    if (memory.type === 'VIDEO') icon = '🎬';
                    if (memory.type === 'MEME') icon = '😂';
                    
                    const date = new Date(parseInt(memory.timestamp) * 1000);
                    
                    memoryElement.innerHTML = `
                        <div class="memory-icon">${icon}</div>
                        <div class="memory-info">
                            <h4>${memory.description}</h4>
                            <div class="memory-meta">
                                <span>${memory.type}</span> • 
                                <span>${date.toLocaleDateString()}</span>
                            </div>
                        </div>
                    `;
                    
                    memoriesList.appendChild(memoryElement);
                });
            } else {
                noMemoriesMessage.style.display = 'block';
            }
        })
        .catch(error => {
            console.error('Error loading memories:', error);
            memoriesList.innerHTML = '<p>Error loading memories. Please try again.</p>';
        });
    }
    
    // Load user transactions
    function loadUserTransactions() {
        fetch('/api/transactions', {
            headers: {
                'Authorization': currentUser.token
            }
        })
        .then(response => response.json())
        .then(data => {
            if (data.transactions && data.transactions.length > 0) {
                noTransactionsMessage.style.display = 'none';
                transactionList.innerHTML = '';
                
                data.transactions.forEach(tx => {
                    const txElement = document.createElement('div');
                    txElement.className = 'transaction-item';
                    
                    const date = new Date(parseInt(tx.timestamp) * 1000);
                    const isIncoming = tx.toAddress === currentUser.address;
                    let txType = 'Transfer';
                    
                    if (tx.type === 1) txType = 'Memory Reward';
                    if (!tx.fromAddress) txType = 'Mining Reward';
                    
                    const amountClass = isIncoming ? 'positive' : 'negative';
                    const amountPrefix = isIncoming ? '+' : '-';
                    
                    txElement.innerHTML = `
                        <div class="transaction-details">
                            <p><strong>${txType}</strong></p>
                            <p>${isIncoming ? 'From' : 'To'}: ${isIncoming ? (tx.fromAddress || 'Blockchain') : tx.toAddress}</p>
                            <small>${date.toLocaleString()}</small>
                        </div>
                        <div class="transaction-amount ${amountClass}">
                            ${isIncoming ? amountPrefix : amountPrefix} ${tx.amount.toFixed(2)} AHM
                        </div>
                    `;
                    
                    transactionList.appendChild(txElement);
                });
            } else {
                noTransactionsMessage.style.display = 'block';
            }
        })
        .catch(error => {
            console.error('Error loading transactions:', error);
            transactionList.innerHTML = '<p>Error loading transactions. Please try again.</p>';
        });
    }
    
    // Load blockchain data
    function loadBlockchainData() {
        fetch('/api/blockchain')
        .then(response => response.json())
        .then(data => {
            // Update stats
            if (data.chain) {
                blockCount.textContent = data.chain.length;
                
                let totalTx = 0;
                let totalMemories = 0;
                
                // Clear blocks container
                blocksContainer.innerHTML = '';
                
                // Add blocks
                data.chain.forEach((block, index) => {
                    totalTx += block.transactions.length;
                    
                    // Count memories
                    block.transactions.forEach(tx => {
                        if (tx.type === 1) totalMemories++;
                    });
                    
                    const blockElement = document.createElement('div');
                    blockElement.className = 'block';
                    
                    blockElement.innerHTML = `
                        <div class="block-header">
                            <h4>Block #${block.index}</h4>
                            <small>${new Date(block.timestamp * 1000).toLocaleString()}</small>
                        </div>
                        <div class="block-content">
                            <div><strong>Hash:</strong> <span class="block-hash">${block.hash.substring(0, 20)}...</span></div>
                            <div><strong>Previous Hash:</strong> <span class="block-prev-hash">${block.previousHash.substring(0, 20)}...</span></div>
                            <div><strong>Miner:</strong> ${block.minerAddress || 'Genesis'}</div>
                            <div><strong>Nonce:</strong> ${block.nonce}</div>
                            <div><strong>Transactions:</strong> ${block.transactions.length}</div>
                        </div>
                    `;
                    
                    blocksContainer.appendChild(blockElement);
                });
                
                transactionCount.textContent = totalTx;
                memoryCount.textContent = totalMemories;
            }
        })
        .catch(error => {
            console.error('Error loading blockchain data:', error);
            blocksContainer.innerHTML = '<p>Error loading blockchain data. Please try again.</p>';
        });
    }
    
    // File preview handler
    function handleFilePreview(file) {
        const reader = new FileReader();
        
        reader.onload = function(e) {
            const fileType = file.type.split('/')[0];
            
            previewContainer.innerHTML = '';
            
            if (fileType === 'image') {
                const img = document.createElement('img');
                img.src = e.target.result;
                img.style.maxWidth = '100%';
                img.style.maxHeight = '300px';
                previewContainer.appendChild(img);
            } else if (fileType === 'video') {
                const video = document.createElement('video');
                video.src = e.target.result;
                video.controls = true;
                video.style.maxWidth = '100%';
                video.style.maxHeight = '300px';
                previewContainer.appendChild(video);
            } else {
                previewContainer.innerHTML = `
                    <p>File selected: ${file.name}</p>
                    <p>Size: ${(file.size / 1024).toFixed(2)} KB</p>
                `;
            }
        };
        
        reader.readAsDataURL(file);
    }
    
    // Event Listeners
    navLinks.forEach(link => {
        link.addEventListener('click', function(e) {
            e.preventDefault();
            
            // Skip if disabled
            if (this.classList.contains('disabled')) return;
            
            // Update active class
            navLinks.forEach(item => item.classList.remove('active'));
            this.classList.add('active');
            
            // Show corresponding section
            const targetSection = this.id.replace('nav-', '') + '-section';
            navigateToSection(targetSection);
            
            // Load section-specific data
            if (targetSection === 'dashboard-section' && isLoggedIn) {
                loadUserMemories();
                loadUserTransactions();
            } else if (targetSection === 'blockchain-section') {
                loadBlockchainData();
            }
        });
    });
    
    // Get Started button
    document.getElementById('get-started-button').addEventListener('click', function() {
        if (isLoggedIn) {
            navigateToSection('dashboard-section');
            document.getElementById('nav-dashboard').classList.add('active');
            document.getElementById('nav-home').classList.remove('active');
        } else {
            registerModal.classList.add('active');
        }
    });
    
    // Learn More button
    document.getElementById('learn-more-button').addEventListener('click', function() {
        navigateToSection('about-section');
        navLinks.forEach(item => item.classList.remove('active'));
        document.getElementById('nav-about').classList.add('active');
    });
    
    // Modal handlers
    loginButton.addEventListener('click', function() {
        loginModal.classList.add('active');
    });
    
    registerButton.addEventListener('click', function() {
        registerModal.classList.add('active');
    });
    
    logoutButton.addEventListener('click', function() {
        fetch('/api/logout', {
            method: 'POST',
            headers: {
                'Authorization': currentUser.token
            }
        })
        .then(() => {
            localStorage.removeItem('token');
            localStorage.removeItem('address');
            currentUser = { address: '', token: '' };
            updateAuthUI(false);
        })
        .catch(error => {
            console.error('Logout error:', error);
        });
    });
    
    closeButtons.forEach(button => {
        button.addEventListener('click', function() {
            this.parentElement.parentElement.classList.remove('active');
            
            // Reset forms
            if (this.parentElement.querySelector('form')) {
                this.parentElement.querySelector('form').reset();
            }
            
            // Hide wallet result
            if (document.getElementById('wallet-result')) {
                document.getElementById('wallet-result').style.display = 'none';
            }
        });
    });
    
    // Form submissions
    loginForm.addEventListener('submit', function(e) {
        e.preventDefault();
        
        const address = document.getElementById('login-address').value;
        const privateKey = document.getElementById('login-private-key').value;
        
        fetch('/api/login', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                address: address,
                privateKey: privateKey
            })
        })
        .then(response => {
            if (!response.ok) {
                throw new Error('Login failed');
            }
            return response.json();
        })
        .then(data => {
            // Store token with a consistent format (fix common token issue)
            const token = data.token;
            console.log('Received login token:', token);
            
            currentUser.token = token;
            currentUser.address = data.address;
            
            // Save to localStorage
            localStorage.setItem('token', token);
            localStorage.setItem('address', data.address);
            
            console.log('Authentication completed, token stored:', token);
            
            // Update UI
            updateAuthUI(true);
            
            // Close modal
            loginModal.classList.remove('active');
            loginForm.reset();
            
            // Navigate to dashboard
            navigateToSection('dashboard-section');
            navLinks.forEach(item => item.classList.remove('active'));
            document.getElementById('nav-dashboard').classList.add('active');
            
            // Load user data
            loadUserMemories();
            loadUserTransactions();
        })
        .catch(error => {
            console.error('Login error:', error);
            alert('Login failed. Please check your credentials and try again.');
        });
    });
    
    registerForm.addEventListener('submit', function(e) {
        e.preventDefault();
        
        fetch('/api/register', {
            method: 'POST'
        })
        .then(response => response.json())
        .then(data => {
            // Show wallet result
            document.getElementById('wallet-result').style.display = 'block';
            document.getElementById('new-wallet-address').value = data.address;
            document.getElementById('new-wallet-private-key').value = data.privateKey;
            
            // Hide the form
            registerForm.style.display = 'none';
        })
        .catch(error => {
            console.error('Registration error:', error);
            alert('Failed to create a wallet. Please try again.');
        });
    });
    
    // Copy buttons for wallet details
    document.getElementById('copy-address').addEventListener('click', function() {
        const addressInput = document.getElementById('new-wallet-address');
        addressInput.select();
        document.execCommand('copy');
        this.textContent = 'Copied!';
        setTimeout(() => this.textContent = 'Copy', 2000);
    });
    
    document.getElementById('copy-private-key').addEventListener('click', function() {
        const keyInput = document.getElementById('new-wallet-private-key');
        keyInput.select();
        document.execCommand('copy');
        this.textContent = 'Copied!';
        setTimeout(() => this.textContent = 'Copy', 2000);
    });
    
    // Upload form
    uploadForm.addEventListener('submit', function(e) {
        e.preventDefault();
        
        const type = document.getElementById('memory-type').value;
        const description = document.getElementById('memory-description').value;
        const file = document.getElementById('memory-file').files[0];
        
        if (!file) {
            alert('Please select a file to upload');
            return;
        }
        
        // Read the file as base64 encoded string
        const reader = new FileReader();
        reader.onload = function(event) {
            // Get base64 data (remove the data:image/jpeg;base64, prefix)
            const base64Data = event.target.result.split(',')[1];
            
            // Create a JSON payload
            const jsonData = {
                type: type,
                description: description,
                fileData: base64Data,
                fileName: file.name,
                fileType: file.type
            };
            
            // Make sure we have a valid token
            if (!currentUser.token) {
                console.error('Missing authentication token!');
                alert('You need to be logged in to upload memories');
                return;
            }
            
            console.log('Uploading memory with token:', currentUser.token);
            
            // Double check our authentication state
            if (!isLoggedIn) {
                console.error('User appears to be logged out but has a token');
                alert('Please log in again to upload memories');
                return;
            }
            
            const authToken = currentUser.token.trim();
            console.log('Using cleaned auth token:', authToken);
            
            fetch('/api/upload', {
                method: 'POST',
                headers: {
                    'Authorization': authToken,
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify(jsonData)
            })
            .then(response => {
                if (!response.ok) {
                    // Try to read the error details from the response
                    return response.json().then(errorData => {
                        throw new Error(errorData.details || errorData.error || 'Upload failed');
                    }).catch(() => {
                        throw new Error(`Upload failed with status ${response.status}`);
                    });
                }
                return response.json();
            })
            .then(data => {
                console.log('Upload success:', data);
                alert('Memory uploaded successfully! You earned Ahmiyat coins.');
                uploadForm.reset();
                previewContainer.innerHTML = '<p>Select a file to preview</p>';
                
                // Navigate to dashboard
                navigateToSection('dashboard-section');
                navLinks.forEach(item => item.classList.remove('active'));
                document.getElementById('nav-dashboard').classList.add('active');
                
                // Reload user data
                loadUserMemories();
                loadUserTransactions();
            })
            .catch(error => {
                console.error('Upload error:', error);
                alert('Upload failed: ' + error.message);
            });
        };
        
        // Start reading the file
        reader.readAsDataURL(file);
    });
    
    // File input change
    memoryFile.addEventListener('change', function() {
        if (this.files && this.files[0]) {
            handleFilePreview(this.files[0]);
        }
    });
    
    // Mine button
    mineButton.addEventListener('click', function() {
        if (!isLoggedIn) {
            alert('Please log in to mine blocks');
            return;
        }
        
        this.disabled = true;
        this.textContent = 'Mining...';
        
        fetch('/api/mine', {
            method: 'POST',
            headers: {
                'Authorization': currentUser.token
            }
        })
        .then(response => {
            if (!response.ok) {
                throw new Error('Mining failed');
            }
            return response.json();
        })
        .then(data => {
            alert('Mining successful! New balance: ' + data.balance.toFixed(2) + ' AHM');
            walletBalance.textContent = data.balance.toFixed(2);
            
            // Reload transactions
            loadUserTransactions();
        })
        .catch(error => {
            console.error('Mining error:', error);
            alert('Mining failed. Please try again.');
        })
        .finally(() => {
            this.disabled = false;
            this.textContent = 'Mine Blocks';
        });
    });
    
    // Transfer coins
    document.getElementById('transfer-button').addEventListener('click', function() {
        const toAddress = document.getElementById('transfer-address').value;
        const amount = parseFloat(document.getElementById('transfer-amount').value);
        
        if (!toAddress || isNaN(amount) || amount <= 0) {
            alert('Please enter a valid address and amount');
            return;
        }
        
        fetch('/api/transfer', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'Authorization': currentUser.token
            },
            body: JSON.stringify({
                toAddress: toAddress,
                amount: amount
            })
        })
        .then(response => {
            if (!response.ok) {
                throw new Error('Transfer failed');
            }
            return response.json();
        })
        .then(data => {
            alert('Transfer successful! New balance: ' + data.balance.toFixed(2) + ' AHM');
            walletBalance.textContent = data.balance.toFixed(2);
            
            // Reset form
            document.getElementById('transfer-address').value = '';
            document.getElementById('transfer-amount').value = '';
            
            // Reload transactions
            loadUserTransactions();
        })
        .catch(error => {
            console.error('Transfer error:', error);
            alert('Transfer failed. Please check your balance and try again.');
        });
    });
    
    // Refresh blockchain
    refreshBlockchain.addEventListener('click', function() {
        loadBlockchainData();
    });
    
    // Initial load
    checkAuth();
    
    // Load blockchain data for explorer
    loadBlockchainData();
});