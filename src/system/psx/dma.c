#include "system/psx/all.h"

void psx_dma_update_interrupts(struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp)
{
	struct PSX *psx = (struct PSX *)state;
	struct PSXDMA *dma = &psx->dma;

	bool int_was_set = ((dma->dicr & 0x80000000) != 0);

	if((dma->dicr&0x00008000) != 0) {
		// Force IRQ = 1
		dma->dicr |= 0x80000000;
	} else if((dma->dicr&0x00800000) == 0) {
		// IRQ Enable = 0
		dma->dicr &= ~0x80000000;
	} else if((((dma->dicr>>16)&(dma->dicr>>24))&0x7F) != 0) {
		// General IRQ enabled & IRQ set case
		dma->dicr |= 0x80000000;
	}

	bool int_is_set = ((dma->dicr & 0x80000000) != 0);

	printf("DMA interrupt update: %08X\n", dma->dicr);
	if(int_is_set && !int_was_set) {
		printf("Fire DMA interrupt\n");
		psx->i_stat |= (1<<3); // DMA
	}
}

void psx_dma_predict_irq(struct EmuGlobal *H, struct EmuState *state)
{
	//struct PSX *psx = (struct PSX *)state;
	//struct PSXDMA *dma = &psx->dma;

	// TODO!
}

void psx_dma_run_channel(struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp, int idx)
{
	assert(idx >= 0 && idx < PSX_DMA_CHANNEL_COUNT);
	struct PSX *psx = (struct PSX *)state;
	struct PSXDMA *dma = &psx->dma;
	struct PSXDMAChannel *channel = &dma->channels[idx];

	if(!TIME_IN_ORDER(channel->H.timestamp, timestamp)) {
		return;
	}

	// TODO: a lot of things.
	// also I don't actually know how the timing works here
	// this also doesn't emulate bus hogging yet
	channel->H.timestamp_end = timestamp;
	uint64_t timedelta = (channel->H.timestamp_end - channel->H.timestamp);
	const uint64_t timestep = 11;
	timedelta /= timestep;

	// Is this channel enabled?
	if((channel->d_chcr & (1<<24)) == 0 || (dma->dpcr & (0x8<<(idx*4))) == 0) {
		// No - skip this stuff
		channel->H.timestamp += timedelta*timestep;
		return;
	}

	if(!channel->running)
	{
		channel->running = true;
		channel->xfer_addr = channel->d_madr;
		channel->xfer_tag = channel->d_madr;
		channel->xfer_block_remain = 0;
		int sync_mode = (channel->d_chcr>>9)&0x3;
		if(sync_mode == 0) {
			channel->xfer_addr = channel->d_madr;
			channel->xfer_block_remain = (channel->d_bcr&0xFFFF);
			if(channel->xfer_block_remain == 0) {
				channel->xfer_block_remain = 0x10000;
			}
		}
	}

	while(timedelta >= 1)
	{
		bool skip_dma_step = false;

		if(channel->xfer_block_remain == 0)
		{
			int sync_mode = (channel->d_chcr>>9)&0x3;

			switch(sync_mode)
			{
				case 0: {
					// All-at-once mode
					//channel->d_madr = channel->xfer_addr;
					channel->running = false;
					channel->d_chcr &= ~(1<<24);
					dma->dicr |= 1<<(24+idx);
					printf("DMA All-at-once done: %d\n", idx);
					psx_dma_update_interrupts(H, state, timestamp);

					// return, nothing left to transfer
					channel->H.timestamp += timedelta*timestep;
					return;
				} break;

				case 1: {
					// Block mode
					channel->d_madr = channel->xfer_addr;
					if((channel->d_bcr>>16) == 0) {
						channel->running = false;
						channel->d_chcr &= ~(1<<24);
						dma->dicr |= 1<<(24+idx);
						psx_dma_update_interrupts(H, state, timestamp);

						// return, nothing left to transfer
						channel->H.timestamp += timedelta*timestep;
						return;

					} else {
						channel->d_bcr -= 0x10000;
						channel->xfer_addr = channel->d_madr;
						channel->xfer_block_remain = (channel->d_bcr&0xFFFF);
						if(channel->xfer_block_remain == 0) {
							channel->xfer_block_remain = 0x10000;
						}
					}
				} break;

				case 2: {
					// Linked-list mode
					assert((channel->d_bcr & (1<<1)) == 0); // Negative step not supported (yet?)
					channel->d_madr = channel->xfer_tag&0x00FFFFFF;
					channel->xfer_addr = channel->d_madr;
					//printf("LL %08X\n", channel->xfer_addr);
					if(channel->xfer_addr == 0x00FFFFFF) {
						channel->running = false;
						channel->d_chcr &= ~(1<<24);
						dma->dicr |= 1<<(24+idx);
						psx_dma_update_interrupts(H, state, timestamp);

						// return, nothing left to transfer
						channel->H.timestamp += timedelta*timestep;
						return;
					} else {
						channel->xfer_tag = psx->ram[(channel->xfer_addr&(sizeof(psx->ram)-1))>>2];
						channel->xfer_addr += 4;
						channel->xfer_block_remain = (channel->xfer_tag>>8)&0x00;
						skip_dma_step = true;
					}
				} break;

				default:
					printf("unsupported sync mode %d\n", sync_mode);
					assert(!"unsupported sync mode");
					break;
			}
		}

		if(skip_dma_step)
		{
			timedelta -= 1;
			channel->H.timestamp += timestep;
			continue;
		}

		switch(idx)
		{
			case 2: {
				// GPU
				struct GPU *gpu = &psx->gpu;

				//printf("GPU DMA on\n");

				if((channel->d_chcr & (1<<0)) == 0) {
					// To RAM
					//assert(gpu->xfer_mode == PSX_GPU_XFER_FROM_GPU);
					assert(gpu->xfer_dma == PSX_GPU_DMA_GPUREAD_TO_CPU);

					uint32_t val = psx_gpu_read_gp0(gpu, H, state, channel->H.timestamp);
					psx->ram[(channel->xfer_addr&(sizeof(psx->ram)-1))>>2] = val;
					channel->xfer_addr += (1-(channel->d_chcr&0x2))<<2;
					channel->xfer_block_remain -= 1;

				} else {
					// From RAM
					if(gpu->xfer_mode == PSX_GPU_XFER_TO_GPU) {
						assert(gpu->xfer_dma == PSX_GPU_DMA_CPU_TO_GP0);
					} else {
						assert(gpu->xfer_mode == PSX_GPU_XFER_NONE);
						assert(gpu->xfer_dma == PSX_GPU_DMA_FIFO);
					}
					//assert(!"TODO: DMA2 from RAM");

					uint32_t val = psx->ram[(channel->xfer_addr&(sizeof(psx->ram)-1))>>2];
					printf("W DMA2 %08X %08X -> %08X\n", channel->xfer_addr, channel->xfer_block_remain, val);
					psx_gpu_write_gp0(gpu, H, state, channel->H.timestamp, val);
					channel->xfer_addr += (1-(channel->d_chcr&0x2))<<2;
					channel->xfer_block_remain -= 1;
				}
			} break;

			case 6: {
				// OTC

				if((channel->d_chcr & (1<<0)) == 0) {
					// To RAM
					uint32_t new_addr = channel->xfer_addr-4;
					uint32_t val = (channel->xfer_block_remain == 1 ? 0x00FFFFFF : new_addr) & 0x00FFFFFF;
					psx->ram[(channel->xfer_addr&(sizeof(psx->ram)-1))>>2] = val;
					channel->xfer_addr -= 4;
					channel->xfer_block_remain -= 1;

				} else {
					// From RAM
					assert(!"DMA6 somehow ended up in From-RAM mode");
				}
			} break;


			default: {
				printf("!!! unhandled DMA channel %d\n", idx);
				assert(!"unhandled DMA channel");
				break;
			} break;
		}

		timedelta -= 1;
		channel->H.timestamp += timestep;
	}

	channel->H.timestamp += timedelta*timestep;
}

void psx_dma_run(struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp)
{
	struct PSX *psx = (struct PSX *)state;
	struct PSXDMA *dma = &psx->dma;

	if(!TIME_IN_ORDER(dma->H.timestamp, timestamp)) {
		return;
	}

	// TODO: a lot of things.
	// also I don't actually know how the timing works here
	dma->H.timestamp_end = timestamp;
	uint64_t timedelta = (dma->H.timestamp_end - dma->H.timestamp);
	const uint64_t timestep = 11;
	timedelta /= timestep;

	// Run channels
	for(int idx = 0; idx < PSX_DMA_CHANNEL_COUNT; idx++)
	{
		psx_dma_run_channel(H, state, timestamp, idx);
	}

	dma->H.timestamp += timedelta*timestep;
}

void psx_dma_write_index(struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp, uint32_t addr, uint32_t val, int idx)
{
	assert(idx >= 0 && idx < PSX_DMA_CHANNEL_COUNT);
	struct PSX *psx = (struct PSX *)state;
	struct PSXDMA *dma = &psx->dma;
	struct PSXDMAChannel *channel = &dma->channels[idx];

	switch(addr&0xC)
	{
		case 0x0:
			channel->d_madr = val&0x00FFFFFF;
			return;
		case 0x4:
			channel->d_bcr = val;
			return;
		case 0x8:
			channel->d_chcr = val&0x91770703;
			if(idx == 6) {
				// DMA6 OTC special case:
				// Most bits are hardwired
				channel->d_chcr &= 0x51000000;
				channel->d_chcr |= 0x00000002;
			}
			return;

		case 0xC:
		default:
			// Unknown!
			return;
	}
}

void psx_dma_write(struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp, uint32_t addr, uint32_t val)
{
	struct PSX *psx = (struct PSX *)state;
	struct PSXDMA *dma = &psx->dma;

	psx_dma_run(H, state, timestamp);

	int idx = ((addr>>4)&0x7);
	if(idx < PSX_DMA_CHANNEL_COUNT)
	{
		psx_dma_write_index(H, state, timestamp, addr, val, idx);

	} else {
		switch(addr&0x7C)
		{
			case 0x70:
				dma->dpcr = val;
				break;
			case 0x74:
				dma->dicr &=  ~0x7FFF803F;
				dma->dicr |=   0x00FF803F&val;
				dma->dicr &= ~(0x7F000000&val);
				psx_dma_update_interrupts(H, state, timestamp);

				break;
			default:
				// Unknown!
				return;
		}
	}
}

uint32_t psx_dma_read_index(struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp, uint32_t addr, int idx)
{
	assert(idx >= 0 && idx < PSX_DMA_CHANNEL_COUNT);
	struct PSX *psx = (struct PSX *)state;
	struct PSXDMA *dma = &psx->dma;
	struct PSXDMAChannel *channel = &dma->channels[idx];

	switch(addr&0xC)
	{
		case 0x0:
			return channel->d_madr;
		case 0x4:
			return channel->d_bcr;
		case 0x8:
			return channel->d_chcr;

		default:
			// Unknown!
			return 0;
	}
}

uint32_t psx_dma_read(struct EmuGlobal *H, struct EmuState *state, uint64_t timestamp, uint32_t addr)
{
	struct PSX *psx = (struct PSX *)state;
	struct PSXDMA *dma = &psx->dma;

	psx_dma_run(H, state, timestamp);

	int idx = ((addr>>4)&0x7);
	if(idx < PSX_DMA_CHANNEL_COUNT)
	{
		return psx_dma_read_index(H, state, timestamp, addr, idx);

	} else {
		switch(addr&0x7C)
		{
			case 0x70:
				return dma->dpcr;
			case 0x74:
				return dma->dicr;
			default:
				// Unknown!
				return 0;
		}
	}
}

