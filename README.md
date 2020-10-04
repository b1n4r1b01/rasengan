# rasengan - extract various firmware blobs from iBoot

Since A7 Apple have been shipping iBoots with some co-processor firmwares embedded in them. They have been embedded in different formats (sometimes as a raw firmware or as an lzfse compressed blob in iBoot) which vary accross device and iOS version combinations. So far I've added support for extracting ANS1 (Csi + RTBuddy), ANS2, SMC and PMU firmwares. Here's some summed up info about these firmwares:


| Fimrwares	   			|      SoC 		| 	iOS     |    Aarch      	 |  Format 		|
|:---------------------:|:-------------:|:---------:|:------------------:|:------------:|
| ANS1 (Csi)	   		| 	  A7-A8	    | 7-9   	|   32bit 			 |  Raw 		|
| ANS1 (RTBuddy)	    | 	  A7-A8	  	| 10-12   	|   32bit 	         |  Raw 		|
| ANS2					|    A11-A13  	| 11-14 	| 	64bit 			 |  LZFSE 		|
| SMC (Raw)	   			| 	  A11     	| 11   		|   32bit 			 |  Raw 		|
| SMC 	   				| 	  A11     	| 12-14   	|   32bit 			 |  LZFSE 		|
| SMC 	   				| 	  A12-A13 	|  12-14   	|   64bit 			 |  LZFSE 		|
| PMU 	   				| 	  A13     	|  13-14  	|   32bit 			 |  Raw 		|

From what I've looked into, the extracted PMU firmware will not have the TEXT segment at the very top but a little later and rasengan should print that offset right. It must be noted that there's no documentation on extraction of these firmwares so my method might be wrong but it works(TM). Also with A14 ANS2 is now a seperate IMG4 Mach-O file thus the reduced iBoot size.

## Usage

```
./rasengan -e -i iBoot.n104.RESEARCH_RELEASE.dec
===================================================
		iBoot-6723.0.48
===================================================
AppleStorageProcessorANS2-1161.1.6~46, @ 0x16D9BC(0x19C19D9BC), 64bit, Size: 0xA4BA1, Decompressed Size: 0x15B160
AppleSMCFirmware-2317.0.32.n104.REL, @ 0x217D9F(0x19C247D9F), 64bit, Size: 0x1FE43, Decompressed Size: 0x437E0
ApplePMUFirmware-26.40.1.Zc0b.debug, @ 0x213E10 (0x19C243E10) Size: 0x2250 TEXT@ 0x213F10
```