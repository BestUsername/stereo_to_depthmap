#ifndef PROCESSOR_H
#define PROCESSOR_H

class Processor
{
public:
    Processor();
    virtual ~Processor();

    void process_clip(int start_frame, int end_frame);
};

#endif // PROCESSOR_H
