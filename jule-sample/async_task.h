#ifndef ASYNC_TASK_H
#define ASYNC_TASK_H

#include "FreeRTOS.h"
#include "queue.h"
#include "async_operation.h"

// Function to initialize the async task and its queue
BaseType_t async_task_init(UBaseType_t uxTaskPriority, uint32_t ulQueueLength);

// Function to submit an operation to the async task
BaseType_t async_submit_operation(async_op_t *op, TickType_t xTicksToWait);

// Test wrapper for the async task handler.
// Declared here for use by test harness.
// In a production build, this might be conditionally compiled.
void prv_async_task_handler_test_wrapper(void *pvParameters);

#endif // ASYNC_TASK_H
