#ifndef WAVE_H
#define WAVE_H

void wave_open(int raw);
int  wave_get(short buffer[2][samp_per_frame]);
void wave_close(void);

#endif
