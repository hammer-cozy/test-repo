#include "FreeRTOS.h" // Illustrative: Actual FreeRTOS headers would be used
#include "task.h"     // Illustrative
#include "queue.h"    // Illustrative

#include "../async_operation.h"
#include "../async_task.h"

#include <stdio.h> // For printf_stub

// --- Stubs for FreeRTOS and stdio functionality (for demonstration) ---
#define pdMS_TO_TICKS(xTimeInMs) ((TickType_t)((xTimeInMs) * configTICK_RATE_HZ / 1000))
#define configTICK_RATE_HZ 1000 // Example tick rate
#define tskIDLE_PRIORITY ((UBaseType_t)0U)
#define portMAX_DELAY (TickType_t)0xFFFFFFFFUL

// Simple stub for printf
void printf_stub(const char *format, ...) {
    // In a real system, this would format and output to a console/UART
    // For this example, we'll just acknowledge the call.
    // va_list args;
    // va_start(args, format);
    // vprintf(format, args); // If you link stdio
    // va_end(args);
    // For now, let's do nothing to avoid linking issues in a bare environment
    (void)format;
}

// --- Sample Asynchronous Operation ---

// This structure can be used for parameters if the operation needs more than one.
typedef struct {
    int delay_ms;
    int some_other_data;
} sample_op_params_t;

void sample_long_operation(async_op_t *op) {
    if (op == NULL) {
        printf_stub("sample_long_operation: op is NULL!\n");
        return;
    }

    sample_op_params_t *op_params = (sample_op_params_t *)op->params;

    if (op_params == NULL) {
        printf_stub("sample_long_operation: op_params is NULL!\n");
        op->status = ASYNC_OP_STATUS_FAILED;
        op->error_code = -1; // Indicate missing parameters
        op->result = "Error: Missing parameters";
        return;
    }

    printf_stub("Sample_long_operation: Starting with delay_ms = %d, other_data = %d. op_timeout_ticks = %lu\n",
           op_params->delay_ms, op_params->some_other_data, op->op_timeout_ticks);

    // Simulate work using vTaskDelay (illustrative)
    // In a real FreeRTOS environment, vTaskDelay would be used.
    // For this stub, we'll just assume the delay happened.
    // vTaskDelay(pdMS_TO_TICKS(op_params->delay_ms));

    // Simulate a potential failure condition
    if (op_params->delay_ms > 2000) {
        printf_stub("Sample_long_operation: Operation simulated failure (delay too long).\n");
        op->status = ASYNC_OP_STATUS_FAILED;
        op->error_code = -2; // Custom error code for timeout simulation
        op->result = "Error: Operation took too long (simulated)";
    } else {
        printf_stub("Sample_long_operation: Completed successfully.\n");
        op->status = ASYNC_OP_STATUS_COMPLETED;
        op->error_code = 0;
        // In a real scenario, result might be dynamically allocated or a pointer to a static string.
        // Ensure the lifetime of 'result' is managed correctly.
        // For this example, we'll use a static string.
        op->result = "Operation Succeeded!";
    }
    // The op_timeout_ticks (op->op_timeout_ticks) is for the operation's internal use.
    // For example, if this function was waiting on an external peripheral, it could use
    // this timeout with its own timing mechanism.
}

// --- Sample Callback Function ---
void sample_callback(void *result, int error_code) {
    printf_stub("Sample_callback: Invoked.\n");
    if (error_code == 0) {
        printf_stub("  Operation Succeeded! Result: %s\n", (char *)result);
    } else {
        printf_stub("  Operation Failed! Error Code: %d, Message: %s\n", error_code, (char *)result);
    }
}

// --- User Data for Callback ---
typedef struct {
    int id;
    const char* name;
} callback_user_data_t;

void sample_callback_with_user_data(void *result, int error_code) {
    // This callback expects op->user_data to be populated correctly.
    // However, the op pointer itself is not passed to the callback,
    // so user_data needs to be retrieved differently if it's not part of the 'result'.
    // The current design passes op->user_data to the callback via op->user_data,
    // but this is not explicitly shown here as the callback signature is fixed.
    // This means the async_op_t's user_data is implicitly available if the callback
    // is designed to know about it (e.g. if it's a method of a class that holds user_data,
    // or if a global/static async_op_t instance is used).
    // For this simple example, let's assume a way to get user_data if needed,
    // or acknowledge this is a point for careful design.
    // The current callback signature `void (*callback)(void *result, int error)`
    // does not directly pass `op->user_data`. This needs to be fixed in the framework
    // if user_data is to be passed directly to the callback.
    //
    // **Correction**: The framework's prv_async_task_handler *should* pass op->user_data
    // to the callback if that's the desired design. Let's assume for now the framework
    // would be: op->callback(op->result, op->error_code, op->user_data);
    // For now, this example won't use user_data in the callback due to the current signature.

    printf_stub("Sample_callback_with_user_data: Invoked.\n");
    if (error_code == 0) {
        printf_stub("  Operation Succeeded! Result: %s\n", (char *)result);
    } else {
        printf_stub("  Operation Failed! Error Code: %d, Message: %s\n", error_code, (char *)result);
    }
    // If user_data were accessible:
    // callback_user_data_t* u_data = (callback_user_data_t*)op_user_data_passed_here;
    // printf_stub("  User Data ID: %d, Name: %s\n", u_data->id, u_data->name);
}


// --- Main Task (Simulating a User Task) ---
void main_task(void *pvParameters) {
    (void)pvParameters; // Unused

    printf_stub("Main_task: Initializing async task system...\n");
    BaseType_t init_status = async_task_init(tskIDLE_PRIORITY + 1, 10); // Priority, Queue Length

    if (init_status != pdPASS) {
        printf_stub("Main_task: Failed to initialize async task system. Halting.\n");
        return; // Or handle error appropriately
    }
    printf_stub("Main_task: Async task system initialized.\n");

    // --- Example 1: Simple operation ---
    async_op_t op1;
    sample_op_params_t params1 = { .delay_ms = 500, .some_other_data = 100 };

    op1.operation = sample_long_operation;
    op1.params = &params1;
    op1.callback = sample_callback;
    op1.user_data = NULL; // Not used by sample_callback directly
    op1.op_timeout_ticks = pdMS_TO_TICKS(1000); // Operation's own timeout
    op1.status = ASYNC_OP_STATUS_PENDING; // Will be set by submit, but good practice
    op1.result = NULL;
    op1.error_code = 0;

    printf_stub("Main_task: Submitting operation 1...\n");
    if (async_submit_operation(&op1, pdMS_TO_TICKS(100)) == pdPASS) { // 100ms timeout to queue
        printf_stub("Main_task: Operation 1 submitted successfully.\n");
    } else {
        printf_stub("Main_task: Failed to submit operation 1 (queue full or timeout).\n");
    }

    // --- Example 2: Operation that might "fail" (simulated) ---
    async_op_t op2;
    sample_op_params_t params2 = { .delay_ms = 2500, .some_other_data = 200 }; // This will fail in sample_long_operation

    op2.operation = sample_long_operation;
    op2.params = &params2;
    op2.callback = sample_callback;
    op2.user_data = NULL;
    op2.op_timeout_ticks = pdMS_TO_TICKS(3000);
    op2.status = ASYNC_OP_STATUS_PENDING;
    op2.result = NULL;
    op2.error_code = 0;

    printf_stub("Main_task: Submitting operation 2...\n");
    if (async_submit_operation(&op2, portMAX_DELAY) == pdPASS) { // Wait indefinitely to queue
        printf_stub("Main_task: Operation 2 submitted successfully.\n");
    } else {
        printf_stub("Main_task: Failed to submit operation 2.\n");
    }

    // --- Example 3: Operation with user data (illustrative of intent) ---
    async_op_t op3;
    sample_op_params_t params3 = { .delay_ms = 200, .some_other_data = 300 };
    callback_user_data_t user_data3 = { .id = 123, .name = "Op3Context" };

    op3.operation = sample_long_operation;
    op3.params = &params3;
    op3.callback = sample_callback_with_user_data; // Different callback
    op3.user_data = &user_data3; // Set user data
    op3.op_timeout_ticks = pdMS_TO_TICKS(500);
    op3.status = ASYNC_OP_STATUS_PENDING;
    op3.result = NULL;
    op3.error_code = 0;

    printf_stub("Main_task: Submitting operation 3...\n");
    if (async_submit_operation(&op3, portMAX_DELAY) == pdPASS) {
        printf_stub("Main_task: Operation 3 submitted successfully.\n");
    } else {
        printf_stub("Main_task: Failed to submit operation 3.\n");
    }


    printf_stub("Main_task: Operations submitted. It would now do other things or wait.\n");
    printf_stub("Main_task: Note - callbacks execute in the context of the AsyncTask.\n");
    printf_stub("Main_task: Ensure thread safety if callbacks access shared data.\n");

    // In a real application, the task would likely enter a loop or wait on other events.
    // For this example, we'll just delay to allow async operations to (conceptually) complete.
    // vTaskDelay(pdMS_TO_TICKS(5000)); // Allow time for operations & callbacks
    printf_stub("Main_task: Example finished.\n");
}

// Illustrative main() for a non-FreeRTOS test environment (conceptual)
// In FreeRTOS, main() typically initializes hardware, creates tasks, and starts the scheduler.
/*
int main() {
    // Initialize scheduler (conceptual)
    // xTaskCreate(main_task, "MainTask", configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY + 2, NULL);
    // vTaskStartScheduler();
    printf_stub("This is a conceptual main. In FreeRTOS, tasks are scheduled.\n");
    printf_stub("To run this example, you would typically call main_task as a FreeRTOS task.\n");
    return 0;
}
*/

/*
Notes on the design and example:
1.  Operation Access to async_op_t:
    The `void (*operation)(struct async_op *op)` signature change is crucial.
    The `sample_long_operation` now correctly demonstrates how the operation can:
    - Access its parameters: `op->params`
    - Set its result: `op->result = ...;`
    - Set its status: `op->status = ASYNC_OP_STATUS_COMPLETED;`
    - Set its error code: `op->error_code = ...;`

2.  Callback and User Data:
    The current callback signature `void (*callback)(void *result, int error)` does not
    directly pass `op->user_data`. If `user_data` needs to be directly available to the
    callback without relying on global/static context or embedding it within the `result`
    structure, the framework's `prv_async_task_handler` would need to be modified to call:
    `op->callback(op->result, op->error_code, op->user_data);`
    And the callback signature in `async_op_t` would need to change to:
    `void (*callback)(void *result, int error, void *user_data);`
    This example file notes this point in `sample_callback_with_user_data`.

3.  FreeRTOS Stubs:
    The example includes stubs for FreeRTOS types and functions (`pdMS_TO_TICKS`, `vTaskDelay` etc.)
    to make the code illustrative and runnable in a conceptual way. In a real FreeRTOS
    project, these would be provided by the FreeRTOS kernel.

4.  printf_stub:
    A `printf_stub` is used for output. In an embedded system, this would typically be
    UART or RTT logging.

5.  Memory Management for `async_op_t` and `params`/`result`:
    The example uses stack-allocated `async_op_t` and `params` structures. This is safe
    because `main_task` (conceptually) exists for the duration of the operations.
    In more complex scenarios, these might need to be dynamically allocated or managed
    from a pool, ensuring they remain valid until the callback has completed.
    The `result` in `sample_long_operation` is a string literal, which is safe. If `result`
    were dynamically allocated, the callback or another entity would be responsible for freeing it.
*/
