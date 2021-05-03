# About
<img src="00_doc/pic_google.jpg" width="300"> <img src="00_doc/pic_siri.jpg" width="300"> <img src="00_doc/pic_alexa.jpg" width="300">

- Voice assistant wake words recognition using Raspberry Pi Pico and TensorFlow Lite for Microcontrollers
    - "Ok Google"
    - "Hey Siri"
    - "Alexa"
- Display a recognized word ( logo ) and feature data ( 49 x 40 spectgram )

## Device
- Raspberry Pi Pico
- OLED Display
    - NHD-1.69-160128UGC3
    - Controller: SEPS525
    - https://www.digikey.jp/product-detail/ja/newhaven-display-intl/NHD-1-69-160128UGC3/NHD-1-69-160128UGC3-ND/4756379
    - Note: it may be better to use LCD because the OLED display causes lots of noise and reduces recognition accuracy
- Microphone
    - HiLetgo 2個セット MAX4466調整可能なゲインブレークアウトボードエレクトレットマイクアンプモジュールラズベリーパイマイクロフォンarduinoマイクロフォン 
    - https://www.amazon.co.jp/gp/product/B07DRGF8C2/ref=ppx_yo_dt_b_asin_title_o05_s00?ie=UTF8&psc=1

### Wiring
- [wiring.txt](wiring.txt)


## Build
- Raspberry Pi Pico
    - Follow the steps in README (https://github.com/iwatake2222/pico-work )
    - You may need to modify settings to change the project to be built and run, or use this commit ( 7431cfcebcaddd27e6465c229ba9a8c5f52f68dd )
- PC ( for debugging )
    - Create a project using CMake in this directory
    - A project for Visual Studio 2019 was tested

### How to change input data (for debugging)
- Raspberry Pi Pico
    - input = AdcBuffer. Default
    - input = TestBuffer (Voice of Yes)
    - input = Prepared feature data (Yes, No)
- PC ( for debugging )
    - input = TestBuffer (Voice of Yes). Default
    - input = Prepared feature data (Yes, No)

## Design
### Dataflow
![dataflow](00_doc/dataflow.png)

### Software Design
![design](00_doc/design.png)

- AudioBuffer:
    - provides an interface to access storead audio data in ring block buffer
- RingBlockBuffer:
    - consists of some blocks. The block size is 512 Byte and the size is equal to DMA's transfer size
    - 512 Byte ( 32 msec @16kHz ) is also convenient to work with FeatureProvider which generates feature data from 30 msec of audio data at 20 msec intervals
- AudioProvider:
    - extracts data from the ring block buffer to the local buffer for the requested time
    - converts data from uint8_t to int16_t
    - allocates the data on sequential memory address
        - the local buffer keeps the past data even after the data was read, because duration time to calculate feature is 30 msec but stride time is 20 msec. It means data for the last 10 msec is used at the next calculation
- FeatureProvider:
    - almost the same as the original code

## Performance
- Processing time:
    - Preprocess (retrieving audio data and creating feature data): 8 msec
    - Inference: 61 msec
- Stride for feature data is 20 msec, so 3 ~ 5 slices of feature are drops. It means 70 ~ 110 msec of input voice is missed. Still input voice to generate feature for each process is continuous.
- AudioProvider copies data onto local buffer and converts it from uint8_t to int16_t. It is redundant. However, preprocess time is smaller than inference time and by doing this, I don't need to modify the original code.

## Scripts
- Model training script:
    - [train_micro_speech_model_voice_assistant_wake_word.ipynb](01_script/train_micro_speech_model_voice_assistant_wake_word.ipynb)
    - Original script:
        - https://colab.research.google.com/github/tensorflow/tensorflow/blob/master/tensorflow/lite/micro/examples/micro_speech/train/train_micro_speech_model.ipynb
        - I made some modifications to recognize voice assistant wake words 
- 1 second voice recording script (to create training data):
    - [record_1sec.py](01_script/record_1sec.py)
- Image file to C array (for logo display):
    - [image2array](01_script/image2array)

## Others
- You can find training data here:
    - https://drive.google.com/drive/folders/1sq1OkqCSlopHiiEYyICRycjXMR-0GPFR?usp=sharing
    - Notice: all the voice is mine, so it may not work with others
- Please read README ( https://github.com/iwatake2222/pico-work ) for other information

## Acknowledgements
- Original project:
    - https://github.com/tensorflow/tensorflow/tree/master/tensorflow/lite/micro/examples/micro_speech
