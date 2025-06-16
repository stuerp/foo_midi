#ifndef _LANCZOS_RESAMPLER_H_
#define _LANCZOS_RESAMPLER_H_

#ifdef __cplusplus
extern "C" {
#endif

void midi_lanczos_init();

void * midi_lanczos_resampler_create();
void midi_lanczos_resampler_delete(void *);

void midi_lanczos_resampler_write_sample(void *, float s);
void midi_lanczos_resampler_set_rate( void *, double new_factor );
int midi_lanczos_resampler_ready(void *);
int midi_lanczos_resampler_get_sample_count(void *_r);
float midi_lanczos_resampler_get_sample(void *_r);
void midi_lanczos_resampler_remove_sample(void *);

#ifdef __cplusplus
}
#endif

#endif
