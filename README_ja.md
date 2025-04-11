// Forward declarations of static functions
static void peer_connection_task(void *arg);
static void peer_signaling_task(void *arg);
ESP_LOGI(TAG, "peer_connection_task ended");
    vTaskDelete(nullptr);

    // Clean up WebRTC resources
    peer_signaling_leave_channel();
    peer_connection_close(pc_);
    
    // Clean up tasks if they're still running
    if (ps_task_handle_) {
        vTaskDelete(ps_task_handle_);
        ps_task_handle_ = nullptr;
    }
    
    if (pc_task_handle_) {
        vTaskDelete(pc_task_handle_);
        pc_task_handle_ = nullptr;
    }
    
    // Clean up semaphore
    if (semaphore_) {
        vSemaphoreDelete(semaphore_);
        semaphore_ = nullptr;
    }
    
    // Destroy peer connection
    peer_connection_destroy(pc_);
    pc_ = nullptr;