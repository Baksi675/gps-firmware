/**
 * @file err.h
 * @author github.com/Baksi675
 * @brief System-wide error code definitions.
 *
 * @details
 * This header defines the @ref ERR_te enumeration used as the standard
 * return type across all modules. Every public API function returns one
 * of these codes to indicate success or the category of failure.
 *
 * @version 0.1
 * @date 2026-01-22
 *
 * @copyright Copyright (c) 2026
 *
 */

/**
 * @defgroup err_module Error Codes
 * @brief System-wide error code enumeration.
 * @{
 */

#ifndef ERR_H_
#define ERR_H_

/**
 * @brief Standard return type used by all public API functions.
 *
 * @details
 * Every function that can fail returns an @ref ERR_te value.
 * @ref ERR_OK (0) indicates success; all other values indicate a specific
 * category of failure and are non-zero.
 */
typedef enum {
    ERR_OK,                         /**< Operation completed successfully. */
    ERR_UNKNOWN,                    /**< An unspecified or unexpected error occurred. Typically indicates an invalid subsystem state (e.g. not initialized or already started). */
    ERR_INVALID_ARGUMENT,           /**< One or more arguments passed to the function are invalid (e.g. out-of-range value or unrecognized name). */
    ERR_INVALID_POINTER,            /**< A pointer argument is NULL or otherwise invalid. */
    ERR_INVALID_CONFIGURATION,      /**< A configuration value violates a requirement (e.g. buffer size is not a power of two). */
    ERR_ARRAY_OUT_OF_BOUNDS,        /**< An array index or offset exceeds the bounds of the target array. */
    ERR_UNINITIALZIED_OBJECT,       /**< The object or handle has not been initialized, or could not be found in the internal pool. */
    ERR_INITIALIZATION_FAILURE,     /**< Initialization failed, or the subsystem or handle was already initialized. */
    ERR_DEINITIALIZATION_FAILURE,   /**< Deinitialization failed because the subsystem is still running, not initialized, or the handle was not found. */
    ERR_CATASTROPHIC_FAILURE,       /**< A severe, unrecoverable error has occurred. */
    ERR_DATA_COPY_FAILURE,          /**< A data copy operation did not complete successfully. */
    ERR_DATA_ACQUISITION_FAILURE,   /**< Data could not be acquired from the expected source. */
    ERR_BUFFER_FULL,                /**< A write operation could not complete because the target buffer is full. */
    ERR_NOT_ENOUGH_SPACE,           /**< The internal object pool has no free slots available. */
    ERR_ILLEGAL_ACTION,             /**< The requested operation is not permitted in the current subsystem state (e.g. subsystem is running or handle is not initialized). */
    ERR_UNSUCCESFUL_ACTION,         /**< The requested action was attempted but did not produce the expected result. */
    ERR_TIMEOUT,                    /**< The operation did not complete within the allowed time. */
    ERR_UNRECOGNIZED_HW,            /**< The connected hardware could not be identified or is not supported. */
    ERR_MODULE_ALREADY_INITIALIZED, /**< The module or subsystem has already been initialized and cannot be initialized again without deinitialization. */
    ERR_MODULE_STOPPED,             /**< The operation could not proceed because the module or subsystem is stopped. */
} ERR_te;

#endif

/** @} */