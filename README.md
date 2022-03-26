# Bit Climax 

This project is an experiment in audio and video signal processing.

The result is an interactive installation meant to be projected on a large surface. The participant is asked to say the time after which begins a digital crescendo using the user’s video and audio input, resulting in a _bit climax_.

This project was built using openFrameworks and consisted of three major stages/workstreams:
1. Audio signal processing using a phase vocoder
2. Video signal processing using GLSL and video feedback
3. Bringing those two technical explorations together in a unified aesthetic whole
  
INSERT_FINAL_PROJECT_VIDEO


## Audio Signal Processing
One goal of mine for this project was to gain a better understanding of what “audio programming” entailed. I understood the basics of how audio was represented in digital files, sample rates, frequencies, etc., but not about how to manipulate them further. I found a [series of tutorials](https://www.youtube.com/watch?v=_QX4ZdlsqSQ&list=PLNURizt7mHsJ9EasygZJl7M3e-kAOV9Pa) on YouTube specifically focused on audio programming in openFrameworks with exercises in building my own oscillators, envelopes, audio recorders — overall I left with a much better understanding of the audioIn/audioOut patterns and handling digital representations of sound files in general.

My initial goal was to create harmonic accompaniment to user input, which led me to interpreting digital files in the frequency domain. This train of thought led to me discovering the “phase vocoder”, and I was interested in digging deeper into it (although it wasn’t quite the right tool for my initial goal).

I was lucky to find a [detailed tutorial on YouTube by Bela Studios](https://www.youtube.com/watch?v=2p_-jbl6Dyc) that walked me through the steps of building a phase vocoder, and after a few missteps, I was able to get something (kind of) working. I stuck pretty closely to the architecture described in the video which entailed:
1. Building my own implementation of a circular buffer
2. Implementing a window based overlap-add algorithm for real-time audio processing
3. Processing audio in blocks within a dedicated thread 
4. Getting the FFT representation of each audio block, modifying the audio signal in the frequency domain, and using phase unwrapping and the inverse FFT to return to the time domain
5. Outputting the modified signal to the audioOut buffer

<img width="600" alt="Screen Shot 2022-03-25 at 1 36 21 AM" src="https://user-images.githubusercontent.com/5685294/160041400-8eb5a711-4f12-4e0d-b870-73834b614467.png">

With this infrastructure, I was able to explore several phase vocoder operations such as:
1. Voice robotization
2. Pitch shifting
3. Delay of different spectral bins
4. Cross over of amplitude/phase with different signals (although I couldn’t get an interesting result with this yet)

Overall, this experience taught me a lot about audio signal processing. I feel pretty comfortable with the real-time block based window processing, but still only have a high level understanding of how the phase unwrapping step works. I think further study will allow me to better understand what opportunities I have in the amplitude/phase domain.

<video width="600" src="https://user-images.githubusercontent.com/5685294/160221473-f6d157a6-d70f-4b57-a2d5-e823c6c998f2.mp4">
*Controlling pitch with mouse: white bins are the original signal, blue bins are the modified signal*

## Video Signal Processing
I was using the phase vocoder to distort my voice input, and wanted to pair that with a distorted video input. I also wanted to gain more experience with GLSL, so I took some time to figure out how to set up a shader in openFrameworks including how to pass webcam data through as a texture.

I was lucky to find several GLSL glitch shaders on github/shadertoy which gave me a good start of the aesthetic I was going for. However, I realized that what I really was interested in was some sort of datamoshing/pixel sorting effect. I wasn’t able to find any existing examples of that, so I had to develop my own algorithm of sorts. I decided that to get the feeling of the screen “melting”, I’d benefit from some sort of feedback mechanism which entailed:
1. Displaying the webcam data on the screen
2. Grabbing the pixels from the screen into an image buffer
3. Passing two textures into the shader:
    1. The clean webcam texture from step 1
    2. The feedback screen texture from step 2
4. Mixing these two textures to interesting results in the shader code

The result of this feedback loop was immediately exciting, creating trails as I moved around. It became even cooler when I started adding distortion and blending the results with the glitch code. I feel like I only breached the surface of what can be accomplished with a signal feedback model.

INSERT_VIDEO_FEEBACK_STILL_OR_VIDEO


## Bringing It All Together
This project began mostly as a technical exploration, but in the back of my mind I was thinking I’d need to come up with a creative way to bring it all together. I recognized that I could parameterize the degree of distortion in both the audio and video signal and had a theory that it could be interesting to see those distortions slowly escalate until it felt like the bits in the screen/speaker were _exploding_.

It took quite a bit of fine tuning, and there were many times when the two didn’t quite add up. The experience made me think of the concept of audio-vision as described by Michael Chion and the delicate balance there is in matching audio with visuals. However, I could feel there were times when I achieved synchronicity and my goal was to have that be the case from beginning to end.

Overall, I’d like for the viewer to see their “reflection” in the screen and feel that representation of themselves be deconstructed before their eyes. It is an anxiety-inducing experience, one that hopefully leaves viewers questioning what reality lies behind their likeness in machines.

MAYBE: INSERT_CLIMAX_VIDEO
