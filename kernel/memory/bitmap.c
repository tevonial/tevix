#include <stdint.h>

#include <memory/bitmap.h>

/**
 * Set a bit in a bitmap
 * @param bitmap pointer to bitmap to act on
 * @param n      bit to set
 */
inline void bitmap_set(uint32_t *bitmap, uint32_t n) {
    bitmap[INDEX_TO_BIT(n)] |= (1 << OFFSET_TO_BIT(n));
}

/**
 * Get the value of a bit in a bitmap
 * @param  bitmap pointer to bitmap to act on
 * @param  n      bit to read
 * @return        value of requested bit
 */
inline uint32_t bitmap_get(uint32_t *bitmap, uint32_t n) {
    return bitmap[INDEX_TO_BIT(n)] & (1 << OFFSET_TO_BIT(n));
}

/**
 * Clear a bit in a bitmap
 * @param bitmap pointer to bitmap to act on
 * @param n      bit to clear
 */
inline void bitmap_clear(uint32_t *bitmap, uint32_t n) {
    bitmap[INDEX_TO_BIT(n)] &= ~(1 << OFFSET_TO_BIT(n));
}