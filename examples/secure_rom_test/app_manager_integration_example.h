// app_manager_integration_example.h
// Example of how to integrate SecureROMLoader with existing AppManager
// This shows the integration approach without modifying the core AppManager yet

#pragma once

#include "../../src/system/app_manager.h"
#include "../../src/engine/security/secure_rom_loader.h"

/**
 * Enhanced AppManager with Security Integration
 * 
 * This class shows how the SecureROMLoader can be integrated with the existing
 * AppManager without breaking backward compatibility.
 */
class SecureAppManager {
private:
    AppManager* baseAppManager;           // Existing app manager
    SecureROMLoader* secureLoader;        // Security layer (optional)
    bool securityEnabled;                 // Whether security is active

public:
    SecureAppManager(AppManager* appMgr) 
        : baseAppManager(appMgr), secureLoader(nullptr), securityEnabled(false) {
    }
    
    /**
     * Enable security validation for ROM loading
     * @param loader Initialized SecureROMLoader instance
     */
    void enableSecurity(SecureROMLoader* loader) {
        secureLoader = loader;
        securityEnabled = (loader != nullptr && loader->isInitialized());
        
        if (securityEnabled) {
            WISP_DEBUG_INFO("SECURE_APP_MGR", "Security validation enabled");
        }
    }
    
    /**
     * Disable security validation (for backward compatibility)
     */
    void disableSecurity() {
        securityEnabled = false;
        WISP_DEBUG_INFO("SECURE_APP_MGR", "Security validation disabled");
    }
    
    /**
     * Load app with optional security validation
     * @param appPath Path to the .wisp ROM file
     * @return True if app loaded successfully
     */
    bool loadApp(const std::string& appPath) {
        if (securityEnabled && secureLoader) {
            return loadAppSecurely(appPath);
        } else {
            // Fall back to existing insecure loading
            return baseAppManager->loadApp(appPath);
        }
    }
    
    /**
     * Load app with security validation
     * @param appPath Path to the .wisp ROM file  
     * @return True if app loaded successfully and passed security validation
     */
    bool loadAppSecurely(const std::string& appPath) {
        if (!secureLoader || !secureLoader->isInitialized()) {
            WISP_DEBUG_ERROR("SECURE_APP_MGR", "Security loader not available");
            return false;
        }
        
        WISP_DEBUG_INFO("SECURE_APP_MGR", "Loading ROM with security validation: %s", appPath.c_str());
        
        // Step 1: Pre-validate ROM file
        if (!secureLoader->validateROMFile(appPath)) {
            WISP_DEBUG_ERROR("SECURE_APP_MGR", "ROM failed security validation");
            return false;
        }
        
        // Step 2: Evaluate memory constraints
        DynamicLimits memoryLimits;
        if (!secureLoader->evaluateMemoryLimits(&memoryLimits)) {
            WISP_DEBUG_ERROR("SECURE_APP_MGR", "Failed to evaluate memory limits");
            return false;
        }
        
        // Step 3: Load ROM with security context
        // This would integrate with WispRuntimeLoader to apply security validation
        bool loadResult = baseAppManager->loadApp(appPath);
        
        if (!loadResult) {
            WISP_DEBUG_ERROR("SECURE_APP_MGR", "ROM loading failed");
            return false;
        }
        
        // Step 4: Post-load validation (if needed)
        // This could validate that the loaded ROM matches security expectations
        
        WISP_DEBUG_INFO("SECURE_APP_MGR", "ROM loaded successfully with security validation");
        return true;
    }
    
    // Delegate other methods to base AppManager
    bool isAppRunning() const {
        return baseAppManager->isAppRunning();
    }
    
    void stopApp() {
        baseAppManager->stopApp();
    }
    
    void update() {
        baseAppManager->update();
    }
    
    std::string getCurrentAppName() const {
        return baseAppManager->getCurrentAppName();
    }
    
    void scanForApps() {
        baseAppManager->scanForApps();
    }
    
    /**
     * Get security statistics
     * @return Security validation statistics
     */
    struct SecurityStats {
        uint32_t romsValidated;
        uint32_t romsRejected;
        uint32_t securityViolations;
        uint32_t memoryAdaptations;
        bool securityEnabled;
    };
    
    SecurityStats getSecurityStats() const {
        SecurityStats stats = {};
        
        if (secureLoader) {
            stats.securityViolations = secureLoader->getSecurityViolationCount();
            stats.romsValidated = secureLoader->getValidatedROMCount();
            stats.romsRejected = secureLoader->getRejectedROMCount();
            stats.memoryAdaptations = secureLoader->getMemoryAdaptationCount();
        }
        
        stats.securityEnabled = securityEnabled;
        return stats;
    }
    
    /**
     * Test ROM security validation without actually loading
     * @param appPath Path to ROM file
     * @return Validation result details
     */
    struct ValidationResult {
        bool passed;
        std::string errorMessage;
        uint32_t violationCount;
        bool memoryAdequate;
    };
    
    ValidationResult testROMSecurity(const std::string& appPath) {
        ValidationResult result = {};
        
        if (!secureLoader) {
            result.errorMessage = "Security loader not available";
            return result;
        }
        
        // Test ROM validation
        result.passed = secureLoader->validateROMFile(appPath);
        result.violationCount = secureLoader->getSecurityViolationCount();
        
        if (!result.passed) {
            result.errorMessage = "ROM failed security validation";
        } else {
            // Test memory requirements
            DynamicLimits limits;
            result.memoryAdequate = secureLoader->evaluateMemoryLimits(&limits);
            
            if (!result.memoryAdequate) {
                result.errorMessage = "Insufficient memory for ROM requirements";
            }
        }
        
        return result;
    }
};

/**
 * Example of how this would integrate with the bootloader
 */
class BootloaderSecurityIntegration {
private:
    SecureAppManager secureAppManager;
    SecureROMLoader secureLoader;

public:
    BootloaderSecurityIntegration(AppManager* baseAppMgr) 
        : secureAppManager(baseAppMgr) {
    }
    
    bool initializeSecurity() {
        WISP_DEBUG_INFO("BOOTLOADER_SECURITY", "Initializing security systems...");
        
        // Initialize security loader
        if (!secureLoader.initialize()) {
            WISP_DEBUG_WARNING("BOOTLOADER_SECURITY", "Security initialization failed - using legacy mode");
            return false;
        }
        
        // Enable security for app manager
        secureAppManager.enableSecurity(&secureLoader);
        
        WISP_DEBUG_INFO("BOOTLOADER_SECURITY", "Security systems initialized successfully");
        return true;
    }
    
    SecureAppManager* getSecureAppManager() {
        return &secureAppManager;
    }
    
    SecureROMLoader* getSecureLoader() {
        return &secureLoader;
    }
};

/**
 * Usage example for integration with bootloader phases
 */
void integrateWithBootloaderPhases() {
    // This would be called during PHASE_SERVICE_LOAD in the bootloader
    
    extern AppManager appManager;  // Existing global app manager
    
    // Create security integration
    static BootloaderSecurityIntegration securityIntegration(&appManager);
    
    // Initialize security systems
    bool securityReady = securityIntegration.initializeSecurity();
    
    if (securityReady) {
        // Use secure app manager for ROM loading
        SecureAppManager* secureAppMgr = securityIntegration.getSecureAppManager();
        
        // Example: Load an app securely
        if (secureAppMgr->loadApp("test_app.wisp")) {
            WISP_DEBUG_INFO("BOOTLOADER", "App loaded with security validation");
        }
        
        // Get security statistics
        auto stats = secureAppMgr->getSecurityStats();
        WISP_DEBUG_INFO("BOOTLOADER", "Security stats: %d validated, %d rejected, %d violations",
            stats.romsValidated, stats.romsRejected, stats.securityViolations);
    } else {
        // Fall back to legacy app manager
        WISP_DEBUG_INFO("BOOTLOADER", "Using legacy app loading (no security)");
    }
}
