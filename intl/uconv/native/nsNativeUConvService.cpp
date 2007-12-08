/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2003
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Mike Hommey <mh@glandium.org>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */


#ifdef MOZ_USE_NATIVE_UCONV
#include "nsString.h"
#include "nsIGenericFactory.h"

#include "nsINativeUConvService.h"

#include "nsIUnicodeDecoder.h"
#include "nsIUnicodeEncoder.h"
#include "nsICharRepresentable.h"

#include "nsNativeUConvService.h"
#include "nsAutoPtr.h"

#include <nl_types.h> // CODESET
#include <langinfo.h> // nl_langinfo
#include <iconv.h>    // iconv_open, iconv, iconv_close
#include <errno.h>
#include <string.h>   // memcpy

#ifdef IS_LITTLE_ENDIAN
const char UTF16[] = "UTF-16LE";
#else
const char UTF16[] = "UTF-16BE";
#endif

#define NS_UCONV_CONTINUATION_BUFFER_LENGTH 8

class IConvAdaptor : public nsIUnicodeDecoder, 
                     public nsIUnicodeEncoder, 
                     public nsICharRepresentable
{
public:
    IConvAdaptor();
    virtual ~IConvAdaptor();
    
    nsresult Init(const char* from, const char* to);
    
    NS_DECL_ISUPPORTS
    
    // Decoder methods:
    
    NS_IMETHOD Convert(const char * aSrc, 
                       PRInt32 * aSrcLength, 
                       PRUnichar * aDest, 
                       PRInt32 * aDestLength);
    
    NS_IMETHOD GetMaxLength(const char * aSrc, 
                            PRInt32 aSrcLength, 
                            PRInt32 * aDestLength);
    NS_IMETHOD Reset();
    
    // Encoder methods:
    
    NS_IMETHOD Convert(const PRUnichar * aSrc, 
                       PRInt32 * aSrcLength, 
                       char * aDest, 
                       PRInt32 * aDestLength);
    
    
    NS_IMETHOD Finish(char * aDest, PRInt32 * aDestLength);
    
    NS_IMETHOD GetMaxLength(const PRUnichar * aSrc, 
                            PRInt32 aSrcLength, 
                            PRInt32 * aDestLength);
    
    // defined by the Decoder:  NS_IMETHOD Reset();
    
    NS_IMETHOD SetOutputErrorBehavior(PRInt32 aBehavior, 
                                      nsIUnicharEncoder * aEncoder, 
                                      PRUnichar aChar);
    
    NS_IMETHOD FillInfo(PRUint32* aInfo);
    
    
private:
    iconv_t mConverter;
    PRBool    mReplaceOnError;
    PRUnichar mReplaceChar;
    char mContinuationBuffer[NS_UCONV_CONTINUATION_BUFFER_LENGTH];
    PRInt32 mContinuationLength;

    const char *mFrom, *mTo;
};

NS_IMPL_ISUPPORTS3(IConvAdaptor, 
                   nsIUnicodeEncoder, 
                   nsIUnicodeDecoder,
                   nsICharRepresentable)

IConvAdaptor::IConvAdaptor()
{
    mConverter = 0;
    mReplaceOnError = PR_FALSE;
    mContinuationLength = 0;
}

IConvAdaptor::~IConvAdaptor()
{
    if (mConverter)
        iconv_close(mConverter);
}

nsresult 
IConvAdaptor::Init(const char* from, const char* to)
{
    mFrom = from;
    mTo = to;

    mConverter = iconv_open(to, from);
    if (mConverter == (iconv_t) -1 )    
    {
#ifdef DEBUG
        printf(" * IConvAdaptor - FAILED Initing: %s ==> %s\n", from, to);
#endif
        mConverter = nsnull;
        return NS_ERROR_FAILURE;
    }
    mContinuationLength = 0;

    return NS_OK;
}

// From some charset to UTF-16
nsresult 
IConvAdaptor::Convert(const char * aSrc, 
                     PRInt32 * aSrcLength, 
                     PRUnichar * aDest, 
                     PRInt32 * aDestLength)
{
    nsresult res = NS_OK;
    size_t inLeft, outLeft;
    PRUnichar *out = aDest;

    if (!mConverter) {
        NS_WARNING("Converter Not Initialized");
        return NS_ERROR_NOT_INITIALIZED;
    }

    if (mTo != UTF16) {
        NS_WARNING("Not an UnicodeDecoder");
        return NS_ERROR_UNEXPECTED;
    }

    inLeft = (size_t) *aSrcLength;
    outLeft = (size_t) *aDestLength * sizeof(PRUnichar);

    if (mContinuationLength > 0) {
        PRInt32 bufLength = NS_UCONV_CONTINUATION_BUFFER_LENGTH - mContinuationLength,
                oneChar = 2, continuationLength = mContinuationLength;

#ifdef DEBUG
        printf(" * IConvAdaptor - Have %d bytes in continuation buffer\n", mContinuationLength);
#endif

        bufLength = bufLength > *aSrcLength ? *aSrcLength : bufLength;
        memcpy(&mContinuationBuffer[mContinuationLength],
               aSrc, bufLength);
        bufLength = mContinuationLength + bufLength;

        mContinuationLength = 0; // We don't want to enter an infinite loop

        res = Convert(mContinuationBuffer, &bufLength, aDest, &oneChar);
        switch (res) {
        case NS_OK_UDEC_MOREINPUT: // Contination buffer ended before filling the 2
                                   // output words, with an incomplete sequence, filling
                                   // a new continuation buffer.
            if (bufLength < continuationLength) { // still not enough data
              *aSrcLength = 0;
              *aDestLength = 0;
              return NS_OK_UDEC_MOREINPUT;
            }
            mContinuationLength = 0;
        case NS_OK: // Continuation buffer ended, unlikely (8 input bytes leading
                    // exactly to 2 output words is quite unlikely)
        case NS_OK_UDEC_MOREOUTPUT: // Standard case, we continue with the
                                    // normal conversion
            inLeft = (size_t) *aSrcLength - (bufLength - continuationLength);
            outLeft -= oneChar * sizeof(PRUnichar);
            aSrc += bufLength - continuationLength;
            aDest += oneChar;
            break;
        case NS_ERROR_UDEC_ILLEGALINPUT:
            *aSrcLength = 0; // Corner case: replacement won't be done as
                             // if it were in the middle of the buffer, since
                             // we can't tell the caller the bad character is
                             // at -mContinuationLength
            *aDestLength = 0;
            return res;
        }
    }

    do {
        if ( iconv(mConverter,
                   (char **)&aSrc,
                   &inLeft,
                   (char **)&aDest,
                   &outLeft) == (size_t) -1 ) {
            switch (errno) {
            case EILSEQ: // Invalid multibyte sequence
                if (mReplaceOnError) {
                    *(aDest++) = mReplaceChar;
                    outLeft -= sizeof(PRUnichar);
                    aSrc++;
                    inLeft--;
                    res = NS_OK;
#ifdef DEBUG
                    printf(" * IConvAdaptor - Replacing char in output ( %s -> %s )\n",
                           mFrom, mTo);
#endif
                } else {
#ifdef DEBUG
                    printf(" * IConvAdaptor - Bad input ( %s -> %s )\n",
                           mFrom, mTo);
#endif
                    res = NS_ERROR_UDEC_ILLEGALINPUT;
                }
                break;
            case EINVAL: // Incomplete multibyte sequence
                mContinuationLength = inLeft;
                memmove(mContinuationBuffer, aSrc, inLeft);
#ifdef DEBUG
                printf(" * IConvAdaptor - Incomplete multibyte sequence in input ( %s -> %s )\n",
                       mFrom, mTo);
#endif
                res = NS_OK_UDEC_MOREINPUT;
                break;
            case E2BIG: // Output buffer full
#ifdef DEBUG
                printf(" * IConvAdaptor - Output buffer full ( %s -> %s )\n",
                       mFrom, mTo);
#endif
                res = NS_OK_UDEC_MOREOUTPUT;
                break;
            }
        }
    } while (mReplaceOnError && (res == NS_OK) && (inLeft != 0));

    *aSrcLength -= inLeft;
    *aDestLength -= (outLeft / sizeof(PRUnichar));

    if (out[0] == 0xfeff) // If first character is BOM, remove it.
        memmove(out, &out[1], (--*aDestLength) * sizeof(PRUnichar));

    return res;
}

nsresult
IConvAdaptor::GetMaxLength(const char * aSrc, 
                          PRInt32 aSrcLength, 
                          PRInt32 * aDestLength)
{
    if (!mConverter) {
        NS_WARNING("Converter Not Initialized");
        return NS_ERROR_NOT_INITIALIZED;
    }

    *aDestLength = aSrcLength*4; // sick
#ifdef DEBUG
    printf(" * IConvAdaptor - GetMaxLength %d ( %s -> %s )\n", *aDestLength, mFrom, mTo);
#endif
    return NS_OK;
}


nsresult 
IConvAdaptor::Reset()
{
    if (!mConverter) {
        NS_WARNING("Converter Not Initialized");
        return NS_ERROR_NOT_INITIALIZED;
    }

    iconv(mConverter, NULL, NULL, NULL, NULL);
    mContinuationLength = 0;

#ifdef DEBUG
    printf(" * IConvAdaptor - Reset\n");
#endif
    return NS_OK;
}


// convert unicode data into some charset.
nsresult 
IConvAdaptor::Convert(const PRUnichar * aSrc, 
                     PRInt32 * aSrcLength, 
                     char * aDest, 
                     PRInt32 * aDestLength)
{
    nsresult res = NS_OK;
    size_t inLeft, outLeft;

    if (!mConverter) {
        NS_WARNING("Converter Not Initialized");
        return NS_ERROR_NOT_INITIALIZED;
    }

    if (mFrom != UTF16) {
        NS_WARNING("Not an UnicodeEncoder");
        return NS_ERROR_UNEXPECTED;
    }

    inLeft = (size_t) *aSrcLength * sizeof(PRUnichar);
    outLeft = (size_t) *aDestLength;

    if (mContinuationLength > 0) {
        // If we're continuing, that means we have a word in the buffer, and
        // that we only need one more word, UTF-16 characters being 2 or 4 bytes
        // long.
        PRInt32 bufLength = 2, destLength = *aDestLength;

#ifdef DEBUG
        printf(" * IConvAdaptor - Have %d bytes in continuation buffer\n", mContinuationLength);
#endif

        mContinuationLength = 0; // We don't want to enter an infinite loop

        ((PRUnichar *) mContinuationBuffer)[1] = *aSrc;

        int i;
        for (i = 0; i < 4; i++) {
        printf("%02x ", mContinuationBuffer[i]);
        }
        printf("\n");

        res = Convert((PRUnichar *) mContinuationBuffer, &bufLength, aDest, &destLength);
        switch (res) {
        case NS_OK_UENC_MOREOUTPUT:
        case NS_ERROR_UENC_NOMAPPING:
            *aSrcLength = 0;
            *aDestLength = 0;
            return res;
        case NS_OK:
            printf("NS_OK %d\n", bufLength);
            outLeft -= destLength;
            inLeft -= sizeof(PRUnichar); // We necessarily have consumed 1 word
            aSrc++;
            aDest += destLength;
        }
    }

    do {
        if ( iconv(mConverter,
                   (char **)&aSrc,
                   &inLeft,
                   (char **)&aDest,
                   &outLeft) == (size_t) -1 ) {
            switch (errno) {
            case EILSEQ: // Invalid multibyte sequence ; there's no way
                         // to know if it's invalid input or input that
                         // doesn't have mapping in the output charset,
                         // but we'll assume our input UTF-16 is valid.
                if (mReplaceOnError) {
                    *(aDest++) = (char)mReplaceChar;
                    outLeft--;
                    aSrc++;
                    inLeft -= sizeof(PRUnichar);
                    res = NS_OK;
#ifdef DEBUG
                    printf(" * IConvAdaptor - Replacing char in output ( %s -> %s )\n",
                           mFrom, mTo);
#endif
                } else {
#ifdef DEBUG
                    printf(" * IConvAdaptor - No mapping in output charset ( %s -> %s )\n",
                           mFrom, mTo);
#endif
                    inLeft -= sizeof(PRUnichar);
                    res = NS_ERROR_UENC_NOMAPPING;
                }
                break;
            case EINVAL: // Incomplete UTF-16 sequence. Happens when dealing with characters
                         // outside BMP split between 2 buffers.
#ifdef DEBUG
                printf(" * IConvAdaptor - Incomplete UTF-16 character in input. ( %s -> %s )\n",
                       mFrom, mTo);
#endif
                mContinuationLength = 2;
                ((PRUnichar *) mContinuationBuffer)[0] = *aSrc;
                res = NS_OK_UENC_MOREINPUT;
                break;
            case E2BIG: // Output buffer full
#ifdef DEBUG
                printf(" * IConvAdaptor - Output buffer full ( %s -> %s )\n",
                       mFrom, mTo);
#endif
                res = NS_OK_UENC_MOREOUTPUT;
                break;
            }
        }
    } while (mReplaceOnError && (res == NS_OK) && (inLeft != 0));

    *aSrcLength -= (inLeft / sizeof(PRUnichar));
    *aDestLength -= outLeft;

    return res;
}


nsresult 
IConvAdaptor::Finish(char * aDest, PRInt32 * aDestLength)
{
    PRInt32 length = *aDestLength;

    if (!mConverter) {
        NS_WARNING("Converter Not Initialized");
        return NS_ERROR_NOT_INITIALIZED;
    }

#ifdef DEBUG
    printf(" * IConvAdaptor - Finish\n");
#endif
    *aDestLength = 0;
    // Flush continuation and send replacement character.
    if ((mContinuationLength > 0) && (mReplaceOnError)) {
        if (length > 0 ) {
            *(aDest++) = (char) mReplaceChar;
            *aDestLength = 1;
        } else {
            return NS_OK_UENC_MOREOUTPUT;
        }
    }
    return NS_OK;
}

nsresult 
IConvAdaptor::GetMaxLength(const PRUnichar * aSrc, 
                          PRInt32 aSrcLength, 
                          PRInt32 * aDestLength)
{
    if (!mConverter) {
        NS_WARNING("Converter Not Initialized");
        return NS_ERROR_NOT_INITIALIZED;
    }

    *aDestLength = aSrcLength*4; // sick

    return NS_OK;
}


nsresult 
IConvAdaptor::SetOutputErrorBehavior(PRInt32 aBehavior, 
                                    nsIUnicharEncoder * aEncoder, 
                                    PRUnichar aChar)
{
    if (aBehavior == nsIUnicodeEncoder::kOnError_Signal) {
        mReplaceOnError = PR_FALSE;
        return NS_OK;
    }
    else if (aBehavior != nsIUnicodeEncoder::kOnError_Replace) {
        mReplaceOnError = PR_TRUE;
        mReplaceChar = aChar;
        return NS_OK;
    }

    NS_WARNING("Uconv Error Behavior not support");
    return NS_ERROR_FAILURE;
}

nsresult 
IConvAdaptor::FillInfo(PRUint32* aInfo)
{
#ifdef DEBUG
    printf(" * IConvAdaptor - FillInfo called\n");
#endif
    *aInfo = 0;
    return NS_OK;
}


NS_IMPL_ISUPPORTS1(NativeUConvService, nsINativeUConvService)

NS_IMETHODIMP
NativeUConvService::GetNativeUnicodeDecoder(const char* from,
                                            nsISupports** aResult)
{
    *aResult = nsnull;

    IConvAdaptor* ucl = new IConvAdaptor();
    if (!ucl)
        return NS_ERROR_OUT_OF_MEMORY;

    // Trick to allow conversion of 0x5c into U+005c instead of U+00a5 with glibc's iconv
    if (strcmp(from, "Shift_JIS") == 0)
        from = "sjis-open";

    // Trick to allow claimed iso-8859-1 actually encoded as windows-1252 to be converted flawlessly
    if (strcmp(from, "ISO-8859-1") == 0)
        from = "windows-1252";

    nsresult rv = ucl->Init(from, UTF16);

    if (NS_SUCCEEDED(rv)) {
        NS_ADDREF(*aResult = (nsISupports*)(nsIUnicodeDecoder*)ucl);
    }

    return rv;
}

NS_IMETHODIMP 
NativeUConvService::GetNativeUnicodeEncoder(const char* to,
                                            nsISupports** aResult)
{
    *aResult = nsnull;

    //nsRefPtr<IConvAdaptor> ucl = new IConvAdaptor();
    IConvAdaptor *adaptor=new IConvAdaptor();
    nsCOMPtr<nsISupports> ucl(static_cast<nsIUnicodeEncoder*>(adaptor));
    if (!ucl)
        return NS_ERROR_OUT_OF_MEMORY;

    //nsresult rv = ucl->Init(from, to);
    nsresult rv=adaptor->Init(UTF16, to);
    if (NS_SUCCEEDED(rv))
        NS_ADDREF(*aResult = ucl);

    return rv;
}
#endif
