#ifndef HKEYSTORE_VOLUME_H
#define HKEYSTORE_VOLUME_H

class Volume
{
protected:
   Volume() = default;
   Volume(const Volume&) = delete;
   void operator=(const Volume&) = delete;

public:
   virtual ~Volume() = default;
};

#endif
