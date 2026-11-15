#pragma once

inline int renderAudioFile(juce::AudioProcessor& processor, const juce::File& inputFile, const juce::File& outputFile)
{
    juce::AudioFormatManager formats;formats.registerBasicFormats();
    auto reader=std::unique_ptr<juce::AudioFormatReader>(formats.createReaderFor(inputFile));
    require(reader!=nullptr,"Cannot read input audio");
    require(reader->numChannels>=1&&reader->numChannels<=2,"Only mono/stereo audio is supported");
    require(inputFile.getFullPathName()!=outputFile.getFullPathName(),"Input and output must be different files");
    outputFile.getParentDirectory().createDirectory();
    auto fileStream=std::make_unique<juce::FileOutputStream>(outputFile);
    require(fileStream->openedOk(),"Cannot open audio output");fileStream->setPosition(0);fileStream->truncate();
    juce::WavAudioFormat wav;
#if JUCE_MAJOR_VERSION >= 9
    std::unique_ptr<juce::OutputStream> stream=std::move(fileStream);
    auto writer=wav.createWriterFor(stream,juce::AudioFormatWriterOptions().withSampleRate(reader->sampleRate)
        .withNumChannels(static_cast<int>(reader->numChannels)).withBitsPerSample(32)
        .withSampleFormat(juce::AudioFormatWriterOptions::SampleFormat::floatingPoint));
#else
    auto writer=std::unique_ptr<juce::AudioFormatWriter>(wav.createWriterFor(fileStream.release(),reader->sampleRate,reader->numChannels,32,{},0));
#endif
    require(writer!=nullptr,"Cannot create audio writer");
    processor.setRateAndBufferSizeDetails(reader->sampleRate,512);processor.prepareToPlay(reader->sampleRate,512);
    const int latency=processor.getLatencySamples();
    const juce::int64 length=reader->lengthInSamples+juce::roundToInt(reader->sampleRate*0.5);
    juce::MidiBuffer midi;double energy=0;float peak=0;juce::int64 written=0;
    for(juce::int64 offset=0;offset<length+latency;offset+=512)
    {
        const int count=static_cast<int>(juce::jmin<juce::int64>(512,length+latency-offset));
        juce::AudioBuffer<float> block(2,count);block.clear();
        if(offset<reader->lengthInSamples)
        {
            reader->read(&block,0,static_cast<int>(juce::jmin<juce::int64>(count,reader->lengthInSamples-offset)),offset,true,true);
            if(reader->numChannels==1)block.copyFrom(1,0,block,0,0,count);
        }
        processor.processBlock(block,midi);
        const int skip=static_cast<int>(juce::jmax<juce::int64>(0,latency-offset));
        if(skip>=count)continue;
        const int frames=count-skip;
        for(int i=skip;i<count;++i)for(unsigned int c=0;c<reader->numChannels;++c)
        {
            const float x=block.getSample(static_cast<int>(c),i);require(std::isfinite(x),"Non-finite offline audio");
            energy+=static_cast<double>(x)*x;peak=juce::jmax(peak,std::abs(x));
        }
        require(writer->writeFromAudioSampleBuffer(block,skip,frames),"Audio write failed");written+=frames;
    }
    processor.releaseResources();
    std::cout<<"RENDER samples="<<written<<" sample_rate="<<reader->sampleRate<<" channels="<<reader->numChannels
        <<" latency_removed="<<latency<<" RMS_dBFS="<<10*std::log10(juce::jmax(1e-30,energy/(written*reader->numChannels)))
        <<" peak_dBFS="<<20*std::log10(juce::jmax(1e-15f,peak))<<"\n";
    return 0;
}
