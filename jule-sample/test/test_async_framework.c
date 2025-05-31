// test/test_async_framework.c
#include <stdio.h> // For printf in stubs
#include <string.h> // For strcmp, memcpy
#include <assert.h> // Basic assertions

// --- FreeRTOS Stubs ---
#define pdPASS (1)
#define pdFAIL (0)
#define portMAX_DELAY ((TickType_t)0xFFFFFFFFUL)
#define tskIDLE_PRIORITY (0)
#define configMINIMAL_STACK_SIZE ( ( unsigned short ) 128 )


typedef unsigned long UBaseType_t;
typedef unsigned long TickType_t;
typedef void * QueueHandle_t;
typedef void * TaskHandle_t;

// Mocking FreeRTOS queue functions
static int mock_queue_send_fail = 0;
static int mock_queue_receive_fail = 0;
static async_op_t* mock_op_in_queue = NULL;
static int mock_queue_items_count = 0;
static int mock_queue_create_fail = 0;
static int mock_task_create_fail = 0;
static QueueHandle_t g_mock_queue_handle = (QueueHandle_t)1; // Global mock handle

QueueHandle_t xQueueCreate(UBaseType_t uxQueueLength, UBaseType_t uxItemSize) {
    (void)uxQueueLength; (void)uxItemSize;
    if (mock_queue_create_fail) return NULL;
    mock_queue_items_count = 0; // Reset count on creation
    mock_op_in_queue = NULL;
    return g_mock_queue_handle; // Return a dummy handle
}

BaseType_t xQueueSendToBack(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait) {
    (void)xQueue; (void)xTicksToWait;
    if (mock_queue_send_fail || xQueue == NULL) return pdFAIL;
    // In a real test, you'd manage a list of items.
    // For this outline, we'll just store the last one for basic checks.
    mock_op_in_queue = *(async_op_t**)pvItemToQueue;
    mock_queue_items_count++;
    return pdPASS;
}

BaseType_t xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait) {
    (void)xQueue; (void)xTicksToWait;
    if (mock_queue_receive_fail || mock_queue_items_count == 0 || xQueue == NULL) return pdFAIL;
    if (mock_op_in_queue) {
        *(async_op_t**)pvBuffer = mock_op_in_queue;
        // mock_op_in_queue = NULL; // Simulate item removal, but for multiple receives, this might be an issue.
                                  // Let's assume test handler calls receive once per item submitted.
        mock_queue_items_count--;
        return pdPASS;
    }
    return pdFAIL;
}

void vQueueDelete(QueueHandle_t xQueue) {
    (void)xQueue;
    // mock_op_in_queue = NULL;
    // mock_queue_items_count = 0;
    // g_mock_queue_handle = NULL; // Optional: invalidate handle
}

// Mocking FreeRTOS task functions
TaskHandle_t xTaskCreate(void (*pvTaskCode)(void *), const char * const pcName, UBaseType_t usStackDepth, void *pvParameters, UBaseType_t uxPriority, TaskHandle_t *pxCreatedTask) {
    (void)pcName; (void)usStackDepth; (void)pvParameters; (void)uxPriority;
    if (mock_task_create_fail) return NULL; // Simulate task creation failure
    if (pxCreatedTask) *pxCreatedTask = (TaskHandle_t)1; // Dummy handle
    // Store task code to be called directly by test if needed
    // current_task_code = pvTaskCode;
    return (TaskHandle_t)1;
}

void vTaskDelete(TaskHandle_t xTaskToDelete) { (void)xTaskToDelete; /* no-op in mock */ }
TickType_t xTaskGetTickCount(void) { return 0; } // Basic stub
void vTaskDelay( TickType_t xTicksToDelay ) { (void)xTicksToDelay; /* no-op */ }


// --- Application Headers ---
// Assuming these are in the parent directory
#include "../async_operation.h"
#include "../async_task.h"

// --- Test Globals & Stubs ---
static async_op_t test_op_global; // Renamed to avoid conflict
static int callback_called_count = 0;
static void* last_callback_result = NULL;
static int last_callback_error = -1;
static char test_user_data[] = "TestUser";
// static char test_result_data[] = "OperationSucceeded"; // Not directly used in this version
static int op_param_value = 42;

// Stubs for operations
void dummy_success_operation(async_op_t* op) {
    if (!op) return;
    op->status = ASYNC_OP_STATUS_COMPLETED;
    op->result = op->params; // Echo params as result for simplicity
    op->error_code = 0;
    // if (op->params) { // Simulate using params
    //     int* p_val = (int*)op->params;
    //     printf("Dummy success op processing: %d\n", *p_val);
    // }
}

void dummy_fail_operation(async_op_t* op) {
    if (!op) return;
    op->status = ASYNC_OP_STATUS_FAILED;
    op->result = NULL;
    op->error_code = 123; // Some error code
}

// Stub for callback
void dummy_callback(void* result, int error) {
    callback_called_count++;
    last_callback_result = result;
    last_callback_error = error;
}

// --- Test Setup/Teardown ---
void setup_test_env() {
    // Reset mocks and globals before each test
    mock_queue_send_fail = 0;
    mock_queue_receive_fail = 0;
    mock_op_in_queue = NULL;
    mock_queue_items_count = 0;
    mock_queue_create_fail = 0;
    mock_task_create_fail = 0;
    g_mock_queue_handle = (QueueHandle_t)1; // Ensure it's reset for init tests

    callback_called_count = 0;
    last_callback_result = NULL;
    last_callback_error = -1;

    memset(&test_op_global, 0, sizeof(async_op_t));
    test_op_global.params = &op_param_value; // Default param
    test_op_global.user_data = test_user_data;
    test_op_global.op_timeout_ticks = 100; // Some default
    test_op_global.status = ASYNC_OP_STATUS_PENDING; // Default initial status for a new op
}

// --- Test Cases ---

void test_async_task_init_success() {
    printf("Running test: test_async_task_init_success\n");
    setup_test_env();
    BaseType_t result = async_task_init(tskIDLE_PRIORITY + 1, 10);
    assert(result == pdPASS && "Task init should succeed");
    // In a real test, you'd check if the queue and task were actually created (e.g., via FreeRTOS trace macros or specific mock checks)
    // For now, we rely on the mock returning success.
    // Clean up (if init creates global resources that aren't auto-cleaned by mock)
    // async_task_deinit(); // Hypothetical deinit
    printf("Test passed.\n");
}

void test_async_task_init_queue_fail() {
    printf("Running test: test_async_task_init_queue_fail\n");
    setup_test_env();
    mock_queue_create_fail = 1;
    BaseType_t result = async_task_init(tskIDLE_PRIORITY + 1, 10);
    assert(result == pdFAIL && "Task init should fail on queue creation error");
    printf("Test passed.\n");
}

void test_async_task_init_task_fail() {
    printf("Running test: test_async_task_init_task_fail\n");
    setup_test_env();
    mock_task_create_fail = 1;
    BaseType_t result = async_task_init(tskIDLE_PRIORITY + 1, 10);
    assert(result == pdFAIL && "Task init should fail on task creation error");
    // Assert that queue was deleted if task creation failed (needs mock support or internal state check)
    // This is implicitly tested by the logic in async_task_init that calls vQueueDelete
    printf("Test passed.\n");
}

void test_submit_operation_success() {
    printf("Running test: test_submit_operation_success\n");
    setup_test_env();
    async_task_init(tskIDLE_PRIORITY + 1, 1); // Init with queue size 1
    test_op_global.operation = dummy_success_operation;
    test_op_global.callback = dummy_callback;

    BaseType_t result = async_submit_operation(&test_op_global, 0);
    assert(result == pdPASS && "Submit should succeed");
    assert(test_op_global.status == ASYNC_OP_STATUS_PENDING && "Status should be PENDING");
    assert(mock_queue_items_count == 1 && "Queue should have 1 item");
    assert(mock_op_in_queue == &test_op_global && "Correct op should be in queue mock");
    printf("Test passed.\n");
}

void test_submit_operation_queue_full_fail_no_wait() {
    printf("Running test: test_submit_operation_queue_full_fail_no_wait\n");
    setup_test_env();
    async_task_init(tskIDLE_PRIORITY + 1, 1); // Queue size 1

    async_op_t first_op; // Op to fill the queue
    memset(&first_op, 0, sizeof(async_op_t));
    first_op.operation = dummy_success_operation;
    first_op.status = ASYNC_OP_STATUS_PENDING; // Initial state before submit
    async_submit_operation(&first_op, 0); // Fill the queue, this should succeed

    // Now try to submit another op, mock xQueueSendToBack to fail as if queue is full
    test_op_global.operation = dummy_fail_operation; // Different op
    test_op_global.status = ASYNC_OP_STATUS_PENDING; // Initial state for this op
    mock_queue_send_fail = 1; // Force next send to fail

    BaseType_t result = async_submit_operation(&test_op_global, 0); // 0 ticks to wait

    assert(result == pdFAIL && "Submit should fail if queue is full and no wait time");
    // Status should remain what it was before submit was called if submit fails
    assert(test_op_global.status == ASYNC_OP_STATUS_PENDING && "Status should not change on failed submit");
    printf("Test passed.\n");
}


// --- Test Runner ---
// This would be the prv_async_task_handler. We need to call it manually.
// Declaration from async_task.h (or made visible for tests)
extern void prv_async_task_handler_test_wrapper(void *pvParameters);

void test_operation_execution_success_with_callback() {
    printf("Running test: test_operation_execution_success_with_callback\n");
    setup_test_env();
    async_task_init(tskIDLE_PRIORITY + 1, 1);

    test_op_global.operation = dummy_success_operation;
    test_op_global.callback = dummy_callback;
    test_op_global.params = &op_param_value; // Ensure params are set

    async_submit_operation(&test_op_global, 0);
    assert(mock_op_in_queue == &test_op_global && "Op should be in queue mock");

    // Simulate task execution
    prv_async_task_handler_test_wrapper(NULL);

    assert(callback_called_count == 1 && "Callback should be called once");
    assert(test_op_global.status == ASYNC_OP_STATUS_COMPLETED && "Status should be COMPLETED");
    assert(last_callback_error == 0 && "Callback error code should be 0");
    assert(last_callback_result == &op_param_value && "Callback result should be the params pointer");
    printf("Test passed.\n");
}

void test_operation_execution_fail_with_callback() {
    printf("Running test: test_operation_execution_fail_with_callback\n");
    setup_test_env();
    async_task_init(tskIDLE_PRIORITY + 1, 1);
    test_op_global.operation = dummy_fail_operation;
    test_op_global.callback = dummy_callback;

    async_submit_operation(&test_op_global, 0);

    // Simulate task execution
    prv_async_task_handler_test_wrapper(NULL);

    assert(callback_called_count == 1 && "Callback should be called once for failed op");
    assert(test_op_global.status == ASYNC_OP_STATUS_FAILED && "Status should be FAILED");
    assert(last_callback_error == 123 && "Callback error code should be 123");
    assert(last_callback_result == NULL && "Callback result should be NULL for this dummy fail op");
    printf("Test passed.\n");
}

void test_operation_execution_no_callback() {
    printf("Running test: test_operation_execution_no_callback\n");
    setup_test_env();
    async_task_init(tskIDLE_PRIORITY + 1, 1);
    test_op_global.operation = dummy_success_operation;
    test_op_global.callback = NULL; // No callback

    async_submit_operation(&test_op_global, 0);

    // Simulate task execution - should not crash
    prv_async_task_handler_test_wrapper(NULL);

    assert(callback_called_count == 0 && "Callback should NOT be called");
    assert(test_op_global.status == ASYNC_OP_STATUS_COMPLETED && "Status should be COMPLETED even without callback");
    printf("Test passed.\n");
}


// --- Main for test file (optional, could be part of a test runner) ---
int main() {
    printf("--- Starting Async Framework Tests ---\n");
    test_async_task_init_success();
    test_async_task_init_queue_fail();
    test_async_task_init_task_fail();
    test_submit_operation_success();
    test_submit_operation_queue_full_fail_no_wait();
    test_operation_execution_success_with_callback();
    test_operation_execution_fail_with_callback();
    test_operation_execution_no_callback();
    // Add more tests...
    printf("--- All tests outlined executed. ---\n");
    return 0;
}
