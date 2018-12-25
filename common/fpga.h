//-----------------------------------------------------------------------------
// This code is licensed to you under the terms of the GNU GPL, version 2 or,
// at your option, any later version. See the LICENSE.txt file for the text of
// the license.
//-----------------------------------------------------------------------------

#ifndef __FPGA_H
#define __FPGA_H

#define FPGA_BITSTREAM_FIXED_HEADER_SIZE    sizeof(bitparse_fixed_header)
#define FPGA_INTERLEAVE_SIZE                288
//Cex: Update to PM3-LCD fpga sizes
// Note that The FPGA is 4 times the size of standard PM3, so having two FPGA
//  images is not needed, although combining the images into a single one requires 
//  increasing MUX size to 4 bits
//#define FPGA_CONFIG_SIZE                    42336L  // our current fpga_[lh]f.bit files are 42175 bytes. Rounded up to next multiple of FPGA_INTERLEAVE_SIZE
#define FPGA_CONFIG_SIZE                    169344L  // our current fpga_[lh]f.bit files are 169297 bytes. Rounded up to next multiple of FPGA_INTERLEAVE_SIZE

static const uint8_t bitparse_fixed_header[] = {0x00, 0x09, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x00, 0x00, 0x01};
extern const int fpga_bitstream_num;
extern const char* const fpga_version_information[];

#endif
