#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/fcntl.h>
#include <sys/errno.h>

#include "pup.h"
#include "bls.h"
#include "decryptio.h"
#include "encryptsrv.h"
#include "defines.h"
#include "debug.h"



int verify_segment(const decrypt_state* state, int index, pup_segment* segment, int additional)
{
  int result;
  uint8_t* buffer = NULL;

  posix_memalign((void**)&buffer, 0x4000, segment->compressed_size);
  ssize_t bytesread = readbytes(state, segment->offset, segment->compressed_size, buffer, segment->compressed_size);
  if (bytesread != segment->compressed_size)
  {
     printf_notification("Failed to read segment #%d for verification!\n", index);
     result = -1;
     return result;
  }

  result = encsrv_verify_segment(state->device_fd, index, buffer, segment->compressed_size, additional);
  if (result != 0)
  {
    printf_notification("Failed to verify segment #%d! %d\n", index, result);
    return result;
  }
  return 0;
}

int verify_segments(const decrypt_state* state, pup_segment* segments, int segment_count)
{
  int result = 0;

  for (int i = 0; i < segment_count; i++)
  {
    pup_segment* segment = &segments[i];
    if ((segment->flags & 0xF0000000) == 0xE0000000)
    {
      printf_notification("Verifying segment #%d (%d)... [1]\n", i, segment->flags >> 20);
      result = verify_segment(state, i, segment, 1);
      if (result < 0)
      {
        return result;
      }
    }
  }

  for (int i = 0; i < segment_count; i++)
  {
    pup_segment* segment = &segments[i];
    if ((segment->flags & 0xF0000000) == 0xF0000000)
    {
      printf_notification("Verifying segment #%d (%d)... [0]\n", i, segment->flags >> 20);
      result = verify_segment(state, i, segment, 0);
      if (result < 0)
      {
        return result;
      }
    }
  }
  return 0;
}

int decrypt_segment(const decrypt_state* state, uint16_t index, pup_segment* segment)
{
  int result = -1;
  
  uint8_t* buffer;
  
  posix_memalign((void**)&buffer, 0x4000, segment->compressed_size);

  int is_compressed = (segment->flags & 8) != 0 ? 1 : 0;

  size_t remaining_size = segment->compressed_size;
  if (is_compressed == 1)
  {
    remaining_size &= ~0xFull;
  }

  if (remaining_size > 0)
  {
    size_t padding_size = segment->compressed_size & 0xF;
    size_t encrypted_size = remaining_size;

    if (segment->compressed_size < remaining_size)
    {
      encrypted_size = segment->compressed_size;
    }

    ssize_t bytesread = readbytes(state, segment->offset, encrypted_size, buffer, segment->compressed_size);
    if (bytesread != encrypted_size)
    {
      printf_notification("Failed to read segment #%d!\n", index);
      result = -1;
	    if (buffer != NULL)
  {
    free(buffer);
  }
      return result;
    }

    result = encsrv_decrypt_segment(state->device_fd, index, buffer, encrypted_size);
    if (result != 0)
    {
      
      printf_notification("Failed to decrypt segment #%d! - Error: %d\n", index, result);
	    if (buffer != NULL)
  {
    free(buffer);
  }
      return result;
    }

    int unencrypted_size = remaining_size - padding_size;
    if (is_compressed == 0 || encrypted_size != remaining_size)
    {
      unencrypted_size = encrypted_size;
    }

    ssize_t byteswritten = writebytes(state, segment->offset, unencrypted_size, buffer, segment->compressed_size);
    if (byteswritten != unencrypted_size) {
      printf_notification("Failed to write segment #%d!\n", index);
      result = -1;
	    if (buffer != NULL)
  {
    free(buffer);
  }
      return result;
    }
  }
  return 0;
}

int decrypt_segment_blocks(const decrypt_state * state, uint16_t index, pup_segment* segment,
                           uint16_t table_index, pup_segment* table_segment)
{
  int result = -1;
  uint8_t* table_buffer = NULL;
  uint8_t* block_buffer = NULL;

  size_t table_length = table_segment->compressed_size;
  posix_memalign((void**)&table_buffer, 0x4000, table_length);

  ssize_t bytesread = readbytes(state, table_segment->offset, table_length, table_buffer, table_length);
  if (bytesread != table_length)
  {
    printf_notification("  Failed to read table for segment #%d!\n", index);
    result = -1;
      if (block_buffer != NULL)
  {
    free(block_buffer);
  }

  if (table_buffer != NULL)
  {
    free(table_buffer);
  }

  return result;
  }

  printf_notification("  Decrypting table #%d for segment #%d\n", table_index, index);
  result = encsrv_decrypt_segment(state->device_fd,
                                 table_index, table_buffer, table_length);
  if (result != 0)
  {
    
    printf_notification("  Failed to decrypt table for segment #%d! Error: %d\n", index, result);
      if (block_buffer != NULL)
  {
    free(block_buffer);
  }

  if (table_buffer != NULL)
  {
    free(table_buffer);
  }

  return result;
  }

  int is_compressed = (segment->flags & 8) != 0 ? 1 : 0;

  size_t block_size = 1 << (((segment->flags & 0xF000) >> 12) + 12);
  int block_count = (block_size + segment->uncompressed_size - 1) / block_size;

  size_t tail_size = segment->uncompressed_size % block_size;
  if (tail_size == 0)
  {
    tail_size = block_size;
  }

  pup_block_info* block_info = NULL;
  if (is_compressed == 1)
  {
    size_t valid_table_length = block_count * (32 + sizeof(pup_block_info));
    if (valid_table_length != table_length)
    {
      printf_notification("  Strange segment #%d table: %lu vs %lu\n",
                   index, valid_table_length, table_length);
    }
    block_info = (pup_block_info*)&table_buffer[32 * block_count];
  }

  posix_memalign((void**)&block_buffer, 0x4000, block_size);

  printf_notification("  Decrypting %d blocks...\n   ", block_count);

  int Seeked = 0;
  //GetElapsed(0);

  size_t remaining_size = segment->compressed_size;
  int last_index = block_count - 1;
  const uint32_t mask = (segment->flags2 & 1) ? 0x1ff : 0xf;
  for (int i = 0; i < block_count; i++)
  {
    printf_notification("  Decrypting block %d/%d...\n", i, block_count);

    if ((block_count > 50) && (i % 5 == 0) ) {
       uint32_t percentage = (uint32_t)(((float)i / (float)block_count) * 100.0f);
	   printf_notification("Approximately %d%% complete processing entry %s (%d/%d) from %s", percentage, state->entryname, state->entryid, state->totalentries, state->input_path);
    }

    size_t read_size;
    ssize_t block_offset = 0;

    if (is_compressed == 1)
    {
      pup_block_info* tblock_info = &block_info[i];
      uint32_t unpadded_size = (tblock_info->size & ~mask) - (tblock_info->size & mask);

      read_size = block_size;
      if (unpadded_size != block_size)
      {
        read_size = tblock_info->size;
        if (i != last_index || tail_size != tblock_info->size)
        {
          read_size &= ~mask;
        }
      }

      if (block_info->offset != 0)
      {
          block_offset = tblock_info->offset;
      }

    }
    else
    {
      read_size = remaining_size;
      if (block_size < read_size)
      {
        read_size = block_size;
      }
    }

    size_t SeekTo = DIO_NOSEEK;
    if (Seeked == 0) {
        SeekTo = (block_offset != 0) ? (segment->offset + block_offset) : segment->offset;
    } else {
        SeekTo = (Seeked != 0) ? DIO_NOSEEK : segment->offset;
    }

    int bytesread = readbytes(state, SeekTo, read_size, block_buffer, block_size);
    if (bytesread != read_size)
    {
      printf_notification("  Failed to read block %d for segment #%d! %d\n", i, index, bytesread);
        if (block_buffer != NULL)
  {
    free(block_buffer);
  }

  if (table_buffer != NULL)
  {
    free(table_buffer);
  }

  return result;
    }

    result = encsrv_decrypt_segment_block(state->device_fd, index, i, block_buffer,
					 read_size, table_buffer, table_length);
    if (result < 0)
    {
      int errcode = errno;
      printf_notification("  Failed to decrypt block for segment #%d! Error: %d (%s)\n", index, errcode, strerror(errcode));
      result = 0;
      //return result;
    }

    ssize_t byteswritten = writebytes(state, SeekTo, read_size, block_buffer, block_size);
    if (byteswritten != read_size)
    {
      printf_notification("  Failed to write block %d for segment #%d!\n", i, index);
        if (block_buffer != NULL)
  {
    free(block_buffer);
  }

  if (table_buffer != NULL)
  {
    free(table_buffer);
  }

  return result;
    }

    Seeked = 1;
    remaining_size -= read_size;
  }
  return 0;
}

int find_table_segment(int index, pup_segment* segments, int segment_count,
                       int* table_index)
{
  if (((index | 0x100) & 0xF00) == 0xF00)
  {
    printf_notification("Can't do table for segment #%d\n", index);
    *table_index = -1;
    return -1;
  }

  for (int i = 0; i < segment_count; i++)
  {
    if (segments[i].flags & 1)
    {
      uint32_t id = segments[i].flags >> 20;
      if (id == index)
      {
        *table_index = i;
        return 0;
      }
    }
  }

  return -2;
}

int decrypt_pup_data(const decrypt_state * state)
{
  int result = -1;
  ssize_t bytesread;
  uint8_t* header_data = NULL;

  pup_file_header file_header;
  bytesread = readbytes(state, DIO_BASEOFFSET, sizeof(file_header), &file_header, sizeof(file_header));
  if (bytesread != sizeof(file_header))
  {
    printf_notification("Failed to read PUP entry header!\n");
    	if (header_data != NULL)
	  {
		free(header_data);
	  }
      return result;
  }


  if (file_header.magic != 0xEEF51454) {
    printf_notification("PUP header magic is invalid!\n");
    	if (header_data != NULL)
	  {
		free(header_data);
	  }
      return result;
  }

  int header_size = file_header.unknown_0C + file_header.unknown_0E;

  posix_memalign((void**)&header_data, 0x4000, header_size);
  memcpy(header_data, &file_header, sizeof(file_header));

  size_t tsize = header_size - sizeof(file_header);
  bytesread = readbytes(state, DIO_NOSEEK, tsize, &header_data[sizeof(file_header)], header_size);
  if (bytesread != tsize)
  {
    printf_notification("Failed to read PUP entry header!\n");
    	if (header_data != NULL)
	  {
		free(header_data);
	  }
      return result;
  }

  if ((file_header.flags & 1) == 0)
  {
    printf_notification("Decrypting header...\n");
    result = encsrv_decrypt_header(state->device_fd, header_data,
				   header_size, state->pup_type);
    if (result != 0)
    {
      
      printf_notification("Failed to decrypt header! Error: %d\n", result);
      	if (header_data != NULL)
	  {
		free(header_data);
	  }
      return result;
    }
  }
  else
  {
    printf_notification("Can't decrypt network pup!\n");
    	if (header_data != NULL)
	  {
		free(header_data);
	  }
      return result;
  }

  pup_header* header = (pup_header*)&header_data[0];
  pup_segment* segments = (pup_segment*)&header_data[0x20];

  ssize_t byteswritten = writebytes(state, DIO_BASEOFFSET, header_size, header_data, header_size);
  if (byteswritten != header_size) {
     printf_notification("Failed to write PUP entry header!\n");
     if (header_data != NULL)
	  {
		free(header_data);
	  }
      return result;
  }

  printf_notification("Verifying segments...\n");
  result = verify_segments(state, segments, header->segment_count);
  if (result < 0)
  {
    printf_notification("Failed to verify segments!\n");
	if (header_data != NULL)
	  {
		free(header_data);
	  }
      return result;
  }


  /*for (int i = 0; i < header->segment_count; i++)
  {
    pup_segment* segment = &segments[i];
    printf_notification("%4d i=%4u b=%u c=%u t=%u r=%05X\n",
                  i, segment->flags >> 20,
                  (segment->flags & 0x800) != 0,
                  (segment->flags & 0x8) != 0,
                  (segment->flags & 0x1) != 0,
                   segment->flags & 0xFF7F6);
  }*/


  printf_notification("Decrypting %d segments...\n", header->segment_count);
  for (int i = 0; i < header->segment_count; i++)
  {
    pup_segment* segment = &segments[i];

    uint32_t special = segment->flags & 0xF0000000;
    if (special == 0xE0000000)
    {
      printf_notification("Skipping additional signature segment #%d!\n", i);
      continue;
    }
    else if (special == 0xF0000000)
    {
      printf_notification("Skipping watermark segment #%d!\n", i);
      continue;
    }

    printf_notification("Decrypting segment %d/%d...\n",
                 1 + i, header->segment_count);

    if ((segment->flags & 0x800) != 0)
    {
      int table_index;
      result = find_table_segment(i, segments, header->segment_count, &table_index);
      if (result < 0)
      {
        printf_notification("Failed to find table for segment #%d!\n", i);
        continue;
      }

      result = decrypt_segment_blocks(state, i, segment, table_index, &segments[table_index]);
    }
    else
    {
      result = decrypt_segment(state, i, segment);
    }

    if (result < 0) {
	  if (header_data != NULL)
	  {
		free(header_data);
	  }
      return result;
    }


  }



  return 0;
}

int decrypt_pup(decrypt_state * state, const char * OutputPath)
{

  int result = -1;
  if (OutputPath != NULL) {
      sprintf(state->output_path, OutputPath, state->entryname);
  }
  else
  {
      sprintf(state->output_path, OUTPUTPATH, state->entryname);
  }

  printf_notification("Creating %s...\n", state->output_path);

  state->output_file = open(state->output_path, O_WRONLY | O_CREAT | O_TRUNC, 0777);
  if (state->output_file == -1)
  {
    printf_notification("Failed to open %s!\n", state->output_path);
    if (state->output_file != -1)
	{
		close(state->output_file);
	}
	return result;
  }


  const char * name = state->entryname;

  if (strcmp(name, "PS5UPDATE1.PUP") == 0) // replace with PROSPEROUPDATE1.PUP for legacy < 2.xx pups
    state->pup_type = 1;

  if (strcmp(name, "PS5UPDATE2.PUP") == 0) // replace with PROSPEROUPDATE2.PUP for legacy < 2.xx pups
    state->pup_type = 0;

  if (state->pup_type < 0)
  {
    printf_notification("Don't know the type for %s!\n", state->output_path);
    if (state->output_file != -1)
	{
		close(state->output_file);
	}
	return result;
  }

  decrypt_pup_data(state);
  return 0;
}


int decrypt_pups(const char * InputPath, const char * OutputPath)
{

  int result = -1;
  decrypt_state state = { 0 };
  state.device_fd = -1;

  char * strings = (char*)malloc(2048);
  state.input_path = strings; //512
  state.output_path = strings+512; //512
  state.entryname = strings+1024; //512
  state.notifystr = strings+1536; //512

  uint8_t * header_data = NULL;
  size_t blsinitial = 0x400;

  sprintf(state.input_path, "%s", (InputPath != NULL) ? InputPath : INPUTPATH);

  printf_notification("Opening %s...\n", state.input_path);
  state.input_file = open(state.input_path, O_RDONLY, 0);
  if (state.input_file == -1)
  {
    printf_notification("Failed to open %s!\n", state.input_path);
	  if (header_data != NULL)
	  {
		free(header_data);
	  }

	  if (strings != NULL) {
		free(strings);
	  }

	  if (state.input_file != -1)
	  {
		close(state.input_file);
	  }

	  if (state.device_fd != -1)
	  {
		close(state.device_fd);
	  }
	  return result;
  }

  posix_memalign((void**)&header_data, 0x4000, blsinitial);

  if (header_data == NULL) {
    printf_notification("Failed to allocate memory!\n");
  }

  ssize_t bytesread = readbytes(&state, DIO_RESET, blsinitial, header_data, blsinitial);

  if (bytesread < blsinitial) {
    printf_notification("Failed to read BLS header or BLS header too small!!\n");
	  if (header_data != NULL)
	  {
		free(header_data);
	  }

	  if (strings != NULL) {
		free(strings);
	  }

	  if (state.input_file != -1)
	  {
		close(state.input_file);
	  }

	  if (state.device_fd != -1)
	  {
		close(state.device_fd);
	  }
	  return result;
  }

  bls_header * header = (bls_header*)header_data;

  if (header->magic != 0x32424C53) {
    printf_notification("Invalid BLS Header!\n");
	  if (header_data != NULL)
	  {
		free(header_data);
	  }

	  if (strings != NULL) {
		free(strings);
	  }

	  if (state.input_file != -1)
	  {
		close(state.input_file);
	  }

	  if (state.device_fd != -1)
	  {
		close(state.device_fd);
	  }
	  return result;
  }

  if ((header->file_count < 1) || (header->file_count > 10)) {
     printf_notification("Invalid PUP entry count!\n");
	  if (header_data != NULL)
	  {
		free(header_data);
	  }

	  if (strings != NULL) {
		free(strings);
	  }

	  if (state.input_file != -1)
	  {
		close(state.input_file);
	  }

	  if (state.device_fd != -1)
	  {
		close(state.device_fd);
	  }
	  return result;
  }

  state.totalentries = header->file_count;

  for (uint32_t i = 0; i < header->file_count; i++)
  {
    state.device_fd = open("/dev/pup_update0", O_RDWR, 0);
    if (state.device_fd < 0)
    {
      printf_notification("Failed to open /dev/pup_update0!\n");
	  if (header_data != NULL)
	  {
		free(header_data);
	  }

	  if (strings != NULL) {
		free(strings);
	  }

	  if (state.input_file != -1)
	  {
		close(state.input_file);
	  }

	  if (state.device_fd != -1)
	  {
		close(state.device_fd);
	  }
	  return result;
    }

    printf_notification("Verifying Bls Header...\n");
    int result = encsrv_verify_blsheader(state.device_fd, header_data, blsinitial, 0);

    if (result != 0) {
        
      printf_notification("Failed while verifying Bls Header! Error: %d\n", result);
	  if (header_data != NULL)
	  {
		free(header_data);
	  }

	  if (strings != NULL) {
		free(strings);
	  }

	  if (state.input_file != -1)
	  {
		close(state.input_file);
	  }

	  if (state.device_fd != -1)
	  {
		close(state.device_fd);
	  }
	  return result;
    }

    sprintf(state.entryname, "%s", header->entry_list[i].name);

    state.pup_type = -1;
    state.entryid = i + 1;

    state.input_base_offset = header->entry_list[i].block_offset * 512;

    state.output_file = -1;
    state.output_base_offset = 0;

    printf_notification("Decrypting \"%s\" (%d/%d) from %s...", state.entryname, state.entryid, state.totalentries, state.input_path);
    

    decrypt_pup(&state, OutputPath);

    close(state.device_fd);
    state.device_fd = -1;

  }
  return 0;
}



