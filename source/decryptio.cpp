#include "decryptio.h"
#include "defines.h"
#include "debug.h"

#define chunksize 2097152

ssize_t readbytes(const decrypt_state * state, int offset, size_t bytes, void * buffer, size_t buffersize) {

  if (bytes > buffersize) {
     SOCK_LOG("ReadBytes failed! - Error: Buffer is too small!\n");
     return -1;
  }

  int result = -1;

  if (offset != 0) {

      switch (offset) {
      case DIO_RESET:
          result = lseek(state->input_file, 0, SEEK_SET);
          break;
      case DIO_BASEOFFSET:
          result = lseek(state->input_file, state->input_base_offset, SEEK_SET);
          break;
      default:
          result = lseek(state->input_file, state->input_base_offset + offset, SEEK_SET);
          break;
      }

      if (result == -1) {
          
          SOCK_LOG("ReadBytes seek_set failed! - Error: %d\n", result);
          return -1;
      }

      SOCK_LOG("Seeked to position %d in input file.\n", result);
  }

  size_t rchunksize = (bytes >= chunksize) ? chunksize : bytes;

  size_t bytesread = 0;
  size_t bytesremaining = bytes;

  while (bytesremaining > 0) {
    result = read(state->input_file, (unsigned char*) buffer+bytesread, (bytesremaining >= rchunksize) ? rchunksize : bytesremaining);

    if (result < 1) {
       break;
    }

    bytesread += result;
    bytesremaining -= result;
  }

  if ((result == -1) || (bytesread != bytes)) {
      
      SOCK_LOG("Read failed; Read " SSIZET_FMT " of " SSIZET_FMT "bytes - Result: %d \n", bytesread, bytes,
												  result);
      return -1;
  }

//  SOCK_LOG("Read " SSIZET_FMT " of " SSIZET_FMT " bytes to from input file.\n", bytesread, byte);

  return bytesread;

}


ssize_t writebytes(const decrypt_state * state, int offset, size_t bytes, void * buffer, size_t buffersize) {

  if (bytes > buffersize) {
     SOCK_LOG("WriteBytes failed! - Error: Buffer is too small!\n");
     return -1;
  }

  int result = -1;

  if (offset != 0) {

      switch (offset) {
      case DIO_RESET:
          result = lseek(state->output_file, 0, SEEK_SET);
          break;
      case DIO_BASEOFFSET:
          result = lseek(state->output_file, state->output_base_offset, SEEK_SET);
          break;
      default:
          result = lseek(state->output_file, state->output_base_offset + offset, SEEK_SET);
          break;
      }

      if (result == -1) {
          SOCK_LOG("WriteBytes seek_set failed! - Error: %d\n", result);
          return -1;
      }

      SOCK_LOG("Seeked to position %d in output file.\n", result);
  }

  size_t wchunksize = (bytes >= chunksize) ? chunksize : bytes;

  size_t byteswritten = 0;
  size_t bytesremaining = bytes;

  while (bytesremaining > 0) {
    result = write(state->output_file, (unsigned char*) buffer+byteswritten, (bytesremaining >= wchunksize) ? wchunksize : bytesremaining);

    if (result < 1) {
       break;
    }

    byteswritten += result;
    bytesremaining -= result;
  }

  if ((result == -1) || (byteswritten != bytes)) {
      
      SOCK_LOG("Write failed; Write " SSIZET_FMT " of " SSIZET_FMT "bytes - Result: %d\n", byteswritten,
												    bytes, result);
      return -1;
  }

//  SOCK_LOG("Write " SSIZET_FMT " of " SSIZET_FMT " bytes to output file.\n", byteswritten, bytes);

  return byteswritten;

}


