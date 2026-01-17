// ADF 5355 driver code -- used for reference 
// https://github.com/analogdevicesinc/no-OS/blob/main/drivers/frequency/adf5355/adf5355.h


#ifndef _ADF5356_H
#define _ADF5356_H

void ADF_init(void);
void ADF_set_out_freq(uint32_t); // Frequency (Hz)

#endif // _ADF5356