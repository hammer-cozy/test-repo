# Asynchronous Task Framework Overview

This document describes a lightweight framework for managing asynchronous operations using a dedicated FreeRTOS task. It allows other tasks to offload work and receive notifications upon completion via callbacks.

## Key Components

### 1. `async_op_t` Structure

Defined in `async_operation.h`, this structure encapsulates all information related to a single asynchronous operation.

```c
typedef struct async_op { // Note: actual typedef is async_op_t
    void (*operation)(struct async_op *op); // Function pointer to the operation to execute
    void *params;                          // Parameters for the operation (accessed via op->params in the operation)
    void (*callback)(void *result, int error); // Callback function for completion
    void *user_data;                      // Optional user data passed to the callback
    void *result;                         // Stores the result of the operation
    TickType_t op_timeout_ticks;          // Timeout for the operation itself (to be used by the operation logic)
    async_op_status_t status;              // Current status of the operation
    int error_code;                       // Error code if the operation failed
} async_op_t;
```

-   `operation`: A function pointer to the actual work to be performed. The function will receive a pointer to its own `async_op_t` structure, allowing it to access parameters and set results/status.
-   `params`: A void pointer to any parameters the `operation` function requires.
-   `callback`: A function pointer that will be invoked when the operation is considered complete (either successfully or with an error).
-   `user_data`: An optional void pointer that can be used to pass application-specific context to the `callback` function.
-   `result`: A void pointer where the `operation` function can store its result. This is then passed to the `callback`.
-   `op_timeout_ticks`: Intended for the `operation` function to implement its own timeout logic if needed. The framework itself does not enforce this timeout on the operation.
-   `status`: The current state of the operation, an `async_op_status_t` enum.
-   `error_code`: An integer to store an error code if the operation fails.

### 2. `async_op_status_t` Enum

Defined in `async_operation.h`, this enum represents the possible states of an asynchronous operation:

-   `ASYNC_OP_STATUS_PENDING`: The operation has been submitted but not yet started by the async task.
-   `ASYNC_OP_STATUS_PROCESSING`: The operation is currently being executed by the async task.
-   `ASYNC_OP_STATUS_COMPLETED`: The operation finished successfully.
-   `ASYNC_OP_STATUS_FAILED`: The operation failed.

### 3. `async_task_init()`

```c
BaseType_t async_task_init(UBaseType_t uxTaskPriority, uint32_t ulQueueLength);
```

-   Initializes the asynchronous task framework.
-   Must be called once before any operations can be submitted.
-   `uxTaskPriority`: The priority for the dedicated FreeRTOS task that will execute the operations.
-   `ulQueueLength`: The maximum number of `async_op_t*` pointers that can be pending in the queue.
-   Returns `pdPASS` on successful initialization, `pdFAIL` otherwise (e.g., if queue or task creation fails).

### 4. `async_submit_operation()`

```c
BaseType_t async_submit_operation(async_op_t *op, TickType_t xTicksToWait);
```

-   Submits an asynchronous operation to be processed.
-   `op`: A pointer to an `async_op_t` structure that has been populated by the caller.
-   `xTicksToWait`: The maximum amount of time the calling task should block if the operation queue is full.
    -   If the operation is successfully queued, `op->status` is set to `ASYNC_OP_STATUS_PENDING`, and `pdPASS` is returned.
    -   If the queue is full and `xTicksToWait` elapses, `pdFAIL` is returned, and `op->status` is not changed.
-   The `async_op_t` structure pointed to by `op` must remain valid until its corresponding callback has been executed (or until it's known it won't be queued/executed). Typically, this means it should not be a stack variable if the submitting task might exit scope before the operation completes.

### 5. Callback Functions

Callback functions must have the following signature:

```c
void my_callback_function(void *result, int error_code);
```

-   `result`: The `result` pointer that was set by the `operation` function within the `async_op_t` structure.
-   `error_code`: The `error_code` that was set by the `operation` function.
-   The callback is invoked by the async task after the `operation` function has run and after the `status` (and potentially `result` and `error_code`) have been set by the `operation` itself.
-   It is executed in the context of the async task.

## Operation Function Responsibilities

The function assigned to `op->operation` is responsible for:

1.  **Executing the Core Logic**: Performing the actual asynchronous work.
2.  **Accessing Parameters**: Retrieving its parameters via `op->params` from the passed `async_op_t* op` structure.
3.  **Setting the Result**: If the operation is successful, it should store its result in `op->result`.
4.  **Setting the Final Status**: It *must* set `op->status` to either `ASYNC_OP_STATUS_COMPLETED` or `ASYNC_OP_STATUS_FAILED`.
5.  **Setting the Error Code**: If the operation fails (`ASYNC_OP_STATUS_FAILED`), it should set `op->error_code` to an appropriate value.
6.  **Timeout Management (Optional)**: If the operation needs to support a timeout, it should use `op->op_timeout_ticks` to manage this internally. The framework does not automatically terminate operations.

This design ensures that the state of the operation is correctly updated before the callback (if any) is invoked.
