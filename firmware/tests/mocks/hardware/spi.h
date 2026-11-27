#ifndef MOCK_HARDWARE_SPI_H
#define MOCK_HARDWARE_SPI_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
	int id;
} spi_inst_t;

extern spi_inst_t mock_spi0_inst;
#define spi0 (&mock_spi0_inst)

static inline unsigned int spi_init(spi_inst_t *spi, unsigned int baudrate)
{
	(void)spi;
	return baudrate;
}

int spi_write_blocking(spi_inst_t *spi, const uint8_t *src, size_t len);

#endif // MOCK_HARDWARE_SPI_H
