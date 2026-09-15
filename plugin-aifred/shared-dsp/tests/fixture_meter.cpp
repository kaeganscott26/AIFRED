#include "aifred/Engine.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
using namespace aifred::core;
int main(int argc,char** argv)
{
    if(argc!=2)return 2;
    std::ifstream input(argv[1],std::ios::binary);if(!input)return 2;
    auto engine=std::make_unique<Engine>();auto latest=std::make_unique<EngineSnapshot>();
    engine->prepare(48000,2);
    std::array<float,512> interleaved{};std::array<float,256> l{},r{};const float* data[]={l.data(),r.data()};
    while(input.read(reinterpret_cast<char*>(interleaved.data()),sizeof(interleaved))||input.gcount()>0)
    {
        auto frames=static_cast<int>(input.gcount()/static_cast<std::streamsize>(2*sizeof(float)));
        for(int i=0;i<frames;++i){l[i]=interleaved[2*i];r[i]=interleaved[2*i+1];}
        engine->process(data,2,frames);while(engine->pop(*latest)){}
    }
    if(!input.eof())return 2;
    std::cout<<std::setprecision(10)<<"{\"integrated_lufs\":"<<latest->get(MetricId::integrated).value
        <<",\"true_peak_dbtp\":"<<latest->get(MetricId::truePeak).value<<",\"lra_lu\":"<<latest->get(MetricId::lra).value<<"}\n";
}
