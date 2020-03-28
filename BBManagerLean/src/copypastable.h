#ifndef _COPYPASTABLE_H_
#define _COPYPASTABLE_H_

struct CopyPaste
{
    struct Copyable
    {
        virtual bool copy() = 0;
    };

    struct Pastable
    {
        virtual bool paste() = 0;
    };
};

#endif // _COPYPASTABLE_H_
