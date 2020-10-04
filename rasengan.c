#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "lzfse/src/lzfse.h"

typedef struct 
{
	void *adr;
	uint32_t sz;
	char *name;
	uint64_t badr;
	int extraction_status;
	int analysis;
}afile;

afile iboot;

typedef struct{
	uint64_t pmu_fw;
	uint32_t pmu_sz;
	uint32_t pmu_uuid_offset;
}pmu_13;

typedef struct {
	uint64_t pmu_fw;
	uint64_t pmu_fw_;
	uint64_t pmu_fw_end;
	uint64_t unk;
	uint32_t pmu_sz;
	uint32_t pmu_uuid_offset;
}pmu_14;

#define OFF(a) (a - iboot.adr)
#define BASE(a) (OFF(a) + iboot.badr)

afile blobfopen(afile input){
	FILE *fp = fopen(input.name, "r");
	if (!fp){
		printf("[!] Can't open file %s\n", input.name);
		exit(1);
	}
	fseek(fp, 0, SEEK_END);
	uint32_t sz = ftell(fp);
	void *mem = malloc(sz);
	if(!mem){
		printf("Can't alloc memory\n");
		exit(1);
	}
	fseek(fp, 0, SEEK_SET);
	fread(mem, sz, 1, fp);
	fclose(fp);
	input.adr = mem;
	input.sz = sz;
	input.badr = 0;
	return input;
}

char *blobfwrite(uint32_t sz, void *ptr, char *name, char **aarch){
	uint64_t ANS = 0x6F7453656C707041;
	uint64_t SMC = 0x434D53656C707041;
	uint64_t PMU = 0x554D50656C707041;
	if(!name){
		name = memmem(ptr, sz, &ANS, sizeof(ANS));
		if(!name){
			name = memmem(ptr, sz, &SMC, sizeof(SMC));
			if(!name){
				name = memmem(ptr, sz, &PMU, sizeof(PMU));
				if(!name)
					name = "unknown image";
			}
		}
	}
	if(iboot.extraction_status){
		FILE *fp = fopen(name, "w");
		fwrite(ptr, sz, 1, fp);
		if(fclose(fp)){
			printf("Failed to save %s\n", name);
		}
	}
	if(aarch){
		if(*(uint32_t*)ptr == 0xEA000006)
			*aarch = "32bit";
		else if(*(uint32_t*)ptr == 0x14000081)
			*aarch = "64bit";
		else
			*aarch = "Unknown aarch";
	}
	return name;
}

void print_help(){
	printf("\nUsage: rasengan [-a/-e] -i iBoot\n");
	printf("\n\t-a\tPrint info about embedded images\n");
	printf("\t-e\tExtract embedded firmwares if any\n");
}

void base_addr(uint32_t major){
	uint32_t base_addr_offset = 0x318;
	if(major > 5540)
		base_addr_offset = 0x300;
	iboot.badr = *(uint64_t*)(iboot.adr + base_addr_offset);
}

void print_banner(){
	if(*(uint32_t*)(iboot.adr + 0x200) != 0x6F6F4269){
		printf("Input file doesn't seems to be an iBoot image, continuing though\n");
		return;
	}
	char *version = iboot.adr + 0x286;
	base_addr(atoi(version));
	printf("\033[0;32m===================================================\n");
	printf("\t\tiBoot-%s\n", version);
	printf("===================================================\033[0m\n");
}

void lzfse_fw(){
	uint32_t bvxE = 0x24787662, bvx2 =	0x32787662;
	void *compressed_blob_start = NULL, *compressed_blob_end = NULL;
	void *image_start = iboot.adr;
	void *newbase = iboot.adr;
	void *image_end = iboot.adr + iboot.sz;
	uint32_t last_offset = 0;
	while(newbase < image_end){
		compressed_blob_start = memmem(newbase, iboot.sz - last_offset, &bvx2, sizeof(bvx2));
		if(compressed_blob_start){
			uint32_t compressed_blob_start_offset = OFF(compressed_blob_start);
			compressed_blob_end =  memmem(newbase , iboot.sz - last_offset, &bvxE, sizeof(bvxE));
			if(compressed_blob_end){
				uint32_t compressed_blob_end_offset = OFF(compressed_blob_end) + 0x4;
				uint32_t compressed_size = compressed_blob_end_offset - compressed_blob_start_offset;
				uint32_t uncompressed_size = compressed_size * 4;
				void *decompressed_blob = malloc(uncompressed_size);
				uint32_t decompressed_size = lzfse_decode_buffer(decompressed_blob, uncompressed_size, compressed_blob_start, compressed_size, NULL);
				if(!decompressed_size){
					printf("Failed to decompress lzfse blob\n");
					free(decompressed_blob);
					return;
				}
				char *aarch;
				char *name = blobfwrite(decompressed_size, decompressed_blob, NULL, &aarch);
				printf("\033[0;32m%s\033[0m, @ 0x%X(0x%llX), %s, Size: 0x%X, Decompressed Size: 0x%X\n",  name, compressed_blob_start_offset, compressed_blob_start_offset + iboot.badr, aarch, compressed_size, decompressed_size);
				newbase = image_start + compressed_blob_end_offset;
				last_offset = compressed_blob_end_offset;
				free(decompressed_blob);
			}
		}
		else
			return;
	}
}

void pmu_fw(){
	uint64_t pmu_magic = 0x6800481200000000;
	uint32_t top_of_fw = 0;
	void *ptr_to_pmu_text = memmem(iboot.adr, iboot.sz, &pmu_magic, sizeof(pmu_magic));
	if(ptr_to_pmu_text){
		ptr_to_pmu_text += 4;
		void *ptr_to_pmu = ptr_to_pmu_text;
		while(top_of_fw != 0x20000101){
			top_of_fw = *(uint32_t*)ptr_to_pmu;
			ptr_to_pmu -= 0x4;
		}
		uint64_t iboot_addr_pmu_fw = BASE(ptr_to_pmu);
		void *xref_to_pmu_fw = memmem(iboot.adr, iboot.sz, &iboot_addr_pmu_fw, sizeof(iboot_addr_pmu_fw));
		if(xref_to_pmu_fw){
			uint32_t pmu_fw_sz = 0;
			if(*(uint64_t*)xref_to_pmu_fw == *(uint64_t*)(xref_to_pmu_fw + 8))
				pmu_fw_sz = ((pmu_14*)xref_to_pmu_fw)->pmu_sz;
			else
				pmu_fw_sz = ((pmu_13*)xref_to_pmu_fw)->pmu_sz;
			char *name = blobfwrite(pmu_fw_sz, ptr_to_pmu, NULL, NULL);
			printf("\033[0;32m%s\033[0m, @ 0x%lX (0x%llX) Size: 0x%X TEXT@ 0x%lX\n", name, OFF(ptr_to_pmu), BASE(ptr_to_pmu), pmu_fw_sz, OFF(ptr_to_pmu_text));
		}
	}
}

void raw_fw(){
	uint32_t rv_insn = 0xEAFFFFFE;
	uint32_t size = iboot.sz, offset = 0, good_name = 1;
	void *newbase = iboot.adr, *iboot_end = newbase + size, *rawfw_rv = NULL, *rawfw_start = NULL;
	rawfw_rv = memmem(newbase, size - offset, &rv_insn, sizeof(rv_insn));
	if(rawfw_rv){
		int pc = 0;
		offset = rawfw_rv - newbase;
		while(pc < 0x100){
			if(*(uint32_t *)(rawfw_rv - pc) == 0){
				rawfw_start = rawfw_rv - pc + 4;
				uint32_t off = OFF(rawfw_start);
				uint32_t rfw_sz = size - off;
				char *fw_name = blobfwrite(rfw_sz, rawfw_start, NULL, NULL);
				printf("\033[0;32m%s\033[0m (Raw Image), @ 0x%X(0x%llX), 32-bit, Size: 0x%X\n", fw_name, off, off + iboot.badr, rfw_sz);
				return;
			}
			pc += 0x4;
		}
	}
}

void find_fw(){
	lzfse_fw();
	pmu_fw();
	raw_fw();
}

int main(int argc, char **argv){

	int temp_arg = argc - 1;
	char *input_file, *output_file;

	while(temp_arg){
		if(*argv[temp_arg] == '-'){
			char arg = *(argv[temp_arg]+ 1);
			switch(arg){
				case('i'):
					iboot.name = argv[temp_arg + 1];
					iboot = blobfopen(iboot);
					break;

				case('e'):
					iboot.extraction_status = 1;
					break;

				case('a'):
					iboot.analysis = 1;
					break;

				case('h'):
					print_help();
					return 1;
			}
		}
		temp_arg--;
	}

	if(!iboot.adr){
		print_help();
		return 1;
	}
	print_banner();
	find_fw();
	free(iboot.adr);
}