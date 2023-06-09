/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef ATTEST_RESULT_INFO_H
#define ATTEST_RESULT_INFO_H

#include "parcel.h"
#include <string>
#include <list>

namespace OHOS {
namespace DevAttest {
class AttestResultInfo : public Parcelable {
public:
    int32_t authResult_ = -1;
    int32_t softwareResult_ = -1;
    std::string ticket_;

    virtual bool Marshalling(Parcel &parcel) const override;
    static sptr<AttestResultInfo> Unmarshalling(Parcel &parcel);
};
} // end of DevAttest
} // end of OHOS
#endif