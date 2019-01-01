#ifndef SMAA_NUKE_H
#define SMAA_NUKE_H

#include "DDImage/PlanarIop.h"
#include "DDImage/Knobs.h"
#include "DDImage/NukeWrapper.h"
#include "DDImage/Blink.h"

#include "Blink/Blink.h"


namespace Nuke {

class Smaa : public DD::Image::PlanarIop
{
public:
    Smaa(Node *node);
    virtual ~Smaa() {}

    const char *Class() const;
    const char *node_help() const;

    static const Iop::Description description;

    int maximum_inputs() const { return 1; }
    int minimum_inputs() const { return 1; }

protected:
    virtual void knobs(DD::Image::Knob_Callback f);
    void _validate(bool);

    void getRequests(
        const DD::Image::Box &box,
        const DD::Image::ChannelSet &channels,
        int count, DD::Image::RequestOutput &data
    ) const;

    void renderStripe(DD::Image::ImagePlane &output_plane);

    void run_edges_detection(
        Blink::ComputeDevice device,
        Blink::Image input,
        Blink::Image edges_tex
    );

    void run_blending_weight_calculation(
        Blink::ComputeDevice device,
        Blink::Image edges_tex,
        Blink::Image blend_tex
    );

    void run_neighborhood_blending(
        Blink::ComputeDevice device,
        Blink::Image input,
        Blink::Image blend_tex,
        Blink::Image output
    );

private:
    Blink::ComputeDevice _gpu_device;
    bool _use_gpu_if_available;

    Blink::ProgramSource _edges_program;
    Blink::ProgramSource _blend_program;
    Blink::ProgramSource _neighborhood_program;
};

} // namespace Nuke

#endif