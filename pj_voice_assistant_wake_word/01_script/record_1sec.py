#-- coding: utf-8 --
# reference: https://tkrel.com/5898

import pyaudio
import wave
import sys
import time

FORMAT = pyaudio.paInt16
CHANNELS = 1
RATE = 16000
CHUNK = 1024
RECORD_SECONDS = 1


frames = []
def callback(in_data, frame_count, time_info, status):
    frames.append(in_data)
    progress = len(frames) / (RATE / CHUNK)
    print("\r" + str(progress), end="")
    return(None, pyaudio.paContinue)

def create_stream(audio):
    stream = audio.open(
        format=FORMAT,
        channels=CHANNELS,
        rate=RATE,
        input=True,
        input_device_index=0,
        frames_per_buffer=CHUNK,
        start=False,
        stream_callback=callback
    )
    return stream

def count_down(duration):
    for i in range(4):
        print("\rReady? " + str(3 - i), end="")
        time.sleep(duration / 4)

if __name__ == '__main__':
    audio = pyaudio.PyAudio()
    file_name_index = 0
    while True:
        frames = []
        filename = format(file_name_index, '08') + ".wav"
        file_name_index = file_name_index + 1
        
        count_down(0.6)
        print("\rStart a recording: " + filename)
        stream = create_stream(audio)
        stream.start_stream()
        time.sleep(RECORD_SECONDS * 1.1)
        stream.stop_stream()
        stream.close()
        print("\r\nStop a recording")

        wf = wave.open(filename, 'wb')
        wf.setnchannels(CHANNELS)
        wf.setsampwidth(audio.get_sample_size(FORMAT))
        wf.setframerate(RATE)
        temp = b''.join(frames)
        wf.writeframes(temp[:RECORD_SECONDS * RATE * 2])
        wf.close()
    audio.terminate()
