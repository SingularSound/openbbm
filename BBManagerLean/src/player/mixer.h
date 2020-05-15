#ifndef MIXER_H_
#define MIXER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*****************************************************************************
 **                     DEFINES
 *****************************************************************************/
// minimum 11.6ms in order for sound card to work properly on mac mini
// buffer needs to be filled by at least 2048 bytes
// 100 was defined arbitrarily to sound good and not to lag too much
// On PC, buffers that are smaller than 80 ms cause distortion at the beginning of song or starving during the whole song.
#    define MIXER_MIN_BUFFERRING_TIME_MS                    ( 50)
#    define MIXER_DEFAULT_BUFFERRING_TIME_MS                (100)
#    define MIXER_MAX_BUFFERRING_TIME_MS                    (500)
#    define MIXER_BUFFERRING_TIME_MS_TO_SMAPLES(time)       (44100 * time / 1000)
#    define MIXER_BUFFER_LENGTH_SAMPLES                     (44100 * MIXER_MAX_BUFFERRING_TIME_MS / 1000)
#    define MIXER_BYTES_PER_SAMPLE_MONO                     (2)
#    define MIXER_BYTES_PER_SAMPLE_STEREO                   (4)
#    define MIXER_BUFFERRING_TIME_MS_TO_BYTES_MONO(time)    (MIXER_BUFFERRING_TIME_MS_TO_SMAPLES(time) * MIXER_BYTES_PER_SAMPLE_MONO)
#    define MIXER_BUFFERRING_TIME_MS_TO_BYTES_STEREO(time)  (MIXER_BUFFERRING_TIME_MS_TO_SMAPLES(time) * MIXER_BYTES_PER_SAMPLE_STEREO)
#    define MIXER_BUFFER_LENGTH_BYTES_MONO                  (MIXER_BUFFER_LENGTH_SAMPLES * MIXER_BYTES_PER_SAMPLE_MONO)
#    define MIXER_BUFFER_LENGTH_BYTES_STEREO                (MIXER_BUFFER_LENGTH_SAMPLES * MIXER_BYTES_PER_SAMPLE_STEREO)


/*****************************************************************************
 **                     FUNCTION PROTOTYPES
 *****************************************************************************/
void mixer_init(void);

void mixer_task(void);

#if !(defined(__x86_64__) || defined(_M_X64))
unsigned int mixer_addPCM16Stereo(unsigned int startAddress,
#else
unsigned int mixer_addPCM16Stereo(uint64_t  startAddress,
#endif
        unsigned int nSample,
        unsigned int vol,
        unsigned int nDelay,
        unsigned int chokeID,
        unsigned int noteID,
        unsigned int fillChokeGroup,
        unsigned int fillChokeDelay,
        unsigned int fillChokeId);

#if !(defined(__x86_64__) || defined(_M_X64))
unsigned int mixer_addPCM24Stereo(unsigned int  startAddress,
#else
unsigned int mixer_addPCM24Stereo(uint64_t  startAddress,
#endif
        unsigned int nSample,
        unsigned int vol,
        unsigned int nDelay,
        unsigned int chokeID,
        unsigned int noteID,
        unsigned int fillChokeGroup,
        unsigned int fillChokeDelay,
        unsigned int fillChokeId);

#if !(defined(__x86_64__) || defined(_M_X64))
unsigned int mixer_addPCM24Mono(unsigned int startAddress,
#else
unsigned int mixer_addPCM24Mono(uint64_t  startAddress,
#endif
        unsigned int nSample,
        unsigned int vol,
        unsigned int nDelay,
        unsigned int chokeID,
        unsigned int noteID,
        unsigned int fillChokeGroup,
        unsigned int fillChokeDelay,
        unsigned int fillChokeId);

#if !(defined(__x86_64__) || defined(_M_X64))
unsigned int mixer_addPCM16Mono(unsigned int startAddress,
#else
unsigned int mixer_addPCM16Mono(uint64_t  startAddress,
#endif
        unsigned int nSample,
        unsigned int vol,
        unsigned int nDelay,
        unsigned int chokeID,
        unsigned int noteID,
        unsigned int fillChokeGroup,
        unsigned int fillChokeDelay,
        unsigned int fillChokeId);

#if !(defined(__x86_64__) || defined(_M_X64))
void mixer_removeSoundWithAddress(unsigned int addr, unsigned int range);
#else
void mixer_removeSoundWithAddress(uint64_t addr, unsigned int range);
#endif

void mixer_removeSoundWithNote(unsigned int note);

void mixer_polyphonyRemove(unsigned int note,
        unsigned int nLimit,
        unsigned int nDelay);

unsigned int mixer_shouldNoteBeExcluded(unsigned int fillChokeGroup, unsigned int fillchokeID);

void mixer_removeAll(void);

void mixer_chokeChannel(unsigned int groupId,unsigned int array_offset);
void mixer_chokeNote(unsigned int note);

float mixer_getOutputLevel(void);
void mixer_setOutputLevel(float level);
void mixer_ReadOutputStream(signed short * buff, unsigned int length);
void mixer_setDitheringBits(int ditheringBits);

void mixer_setLeftFreq(unsigned int freq);
void mixer_setRightFreq(unsigned int freq);
#ifdef __cplusplus
}
#endif

#endif /* MIXER_H_ */
