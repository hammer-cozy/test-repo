#ifndef ASYNC_OPERATION_H
#define ASYNC_OPERATION_H

// Enum for asynchronous operation statuses
typedef enum {
    ASYNC_OP_STATUS_PENDING,
    ASYNC_OP_STATUS_PROCESSING,
    ASYNC_OP_STATUS_COMPLETED,
    ASYNC_OP_STATUS_FAILED
} async_op_status_t;

// Structure for an asynchronous operation
typedef struct async_op { // Added a name for self-reference
    void (*operation)(struct async_op *op);  // Function pointer to the operation to execute
    void *params;                     // Parameters for the operation
    void (*callback)(void *result, int error); // Callback function for completion
    void *user_data;                 // Optional user data to be passed to the callback
    void *result;                    // Result of the operation
    TickType_t op_timeout_ticks;     // Timeout for the operation itself (to be used by the operation)
    async_op_status_t status;         // Status of the operation
    int error_code;                  // Error code if the operation failed
} async_op_t;

#endif // ASYNC_OPERATION_H
