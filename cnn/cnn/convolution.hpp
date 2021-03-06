#ifndef CONVOLUTION_HEADER
#define CONVOLUTION_HEADER

#include "layer.hpp"

#include <string>
#include <cstring>

namespace cnn {
    class ConvolutionLayer : public Layer {
    public:

        ConvolutionLayer(const LayerParam &params,
            const vec &weight, 
            const vec &offset,
            const cl_context &context,
            const cl_program &program,
            const cl_mem &clIn
            ) : Layer(params, weight, offset, context, program, clIn),
            kernelSize(params.kernelSize) {

            // Resize the input buffer.
            inputBuffer.resize(kernelSize * kernelSize);

            // Prepare the ND-Range.
            global[0] = closestMultiple(workGroupSize[0], oWidth);
            global[1] = closestMultiple(workGroupSize[1], oHeight * oDepth);
            global[2] = workGroupSize[2];
        }

        virtual ~ConvolutionLayer() {
        }

        // Forward with CPU.
        virtual unsigned long long forwardCPU(const vec &in) {

            clock_t start = clock(), diff;
            
            // Clear the output buffer.
            std::fill(out.begin(), out.end(), 0.0f);

            // For each output feature map.
            for (size_t o = 0; o < oDepth; ++o) {
                // For each input feature map.
                for (size_t i = 0; i < iDepth; ++i) {
                    // For each element in the output feature map.
                    for (size_t r = 0; r < oHeight; ++r) {
                        for (size_t c = 0; c < oWidth; ++c) {
                            getInput(i, r, c, in);
                            out[getOutputIdx(o, r, c)] += convolution(getWeightBase(i, o));
                        }
                    }
                }

                // Activate function.
                for (size_t r = 0; r < oHeight; ++r) {
                    for (size_t c = 0; c < oWidth; ++c) {
                        size_t idx = getOutputIdx(o, r, c);
                        out[idx] = sigmod(out[idx] + offset[o]);
                    }
                }
            }

            diff = clock() - start;
            int msec = diff * 1000 / CLOCKS_PER_SEC;

            return (unsigned long long)msec;
        }

    private:
        /*****************************************************************************************
         For CPU forward.
         *****************************************************************************************/
        // Prepare the input buffer.
        inline void getInput(size_t i, size_t r, size_t c, const vec &in) {
            size_t idx = 0;
            for (size_t x = 0; x < kernelSize; ++x) {
                for (size_t y = 0; y < kernelSize; ++y) {
                    inputBuffer[idx++] = in[i * iWidth * iHeight + (r + x) * iWidth + c + y];
                }
            }
        }

        // Get the output feature map element index.
        inline size_t getOutputIdx(size_t o, size_t r, size_t c) {
            return o * oWidth * oHeight + r * oWidth + c;
        }

        // Get the base index of the weight.
        inline size_t getWeightBase(size_t i, size_t o) {
            return (o * iDepth + i) * kernelSize * kernelSize;
        }

        // Do the convolution with weight and the input buffer.
        float convolution(size_t weightBase) {
            float sum = 0.0f;
            for (size_t i = 0; i < kernelSize * kernelSize; ++i) {
                sum += weight[weightBase + i] * inputBuffer[i];
            }
            return sum;
        }

        // Kernel size.
        size_t kernelSize;

        // Buffer for convolution.
        vec inputBuffer;

    };

}

#endif