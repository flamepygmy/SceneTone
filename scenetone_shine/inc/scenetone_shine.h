#if !defined(__SCENETONE_SHINE_H__)
#define      __SCENETONE_SHINE_H__

#include <e32base.h>
#include "scenetoneinterfaces.h"

#if defined(SCENETONE_SHINE_IMPLEMENTATION)
extern "C"
{
#define bool bool
#include "Types.h"
#include "Wave.h"
#include "Layer3.h"
}
#endif

class CScenetoneShine : public CBase, public MScenetoneFileWriter
{
  public:
    TInt GetSupportedRates(const TInt *&aSampleRates, const TInt *&aBitRates);
    TInt Start(const TDesC &aOutputFileName, TInt aSampleRate, TInt aChannels, TInt aSamples, TInt aBitRate, TInt (*aCallBack)(TAny *aOutput, TInt aBytes) );

    ~CScenetoneShine();
    static CScenetoneShine *NewL();

   private:
   	
    void   ConstructL();
};

CScenetoneShine *ScenetoneCreateShineWriter();

#endif
