/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#include "devattest_notification_publish.h"

#include <cstdint>
#include <securec.h>
#include "net_conn_client.h"
#include "net_conn_constants.h"
#include "notification_helper.h"
#include "notification_content.h"
#include "notification_normal_content.h"
#include "notification_request.h"
#include "system_ability_definition.h"
#include "system_ability_status_change_stub.h"
#include "iservice_registry.h"
#include "parameter.h"
#include "bundle_mgr_interface.h"
#include "devattest_log.h"
#include "devattest_errno.h"
#include "attest_entry.h"

#define LETTER_PATTERN "[a-zA-Z]"

namespace OHOS {
namespace DevAttest {
using namespace OHOS;
using namespace OHOS::EventFwk;
using namespace std;

constexpr std::int32_t LOCALE_ITEM_SIZE = 5;
constexpr std::int32_t LOCALE_SIZE = 17;
constexpr std::int32_t PARAM_THREE = 3;
constexpr std::int32_t DEVATTEST_PUBLISH_USERID = 0;
constexpr std::int32_t DEVATTEST_PUBLISH_NOTIFICATION_ID = 0;
const char* PERSIST_GLOBAL_LOCALE_LABEL = "persist.global.locale";
const char* DEFAULT_LOCALE_LABEL = "zh-Hans-CN";
const char* DEVATTEST_PUBLISH_BUNDLE = "com.ohos.settingsdata";
const char* DEVATTEST_CONTENT_TITLE = "OpenHarmony_Compatibility_Assessment";
const char* DEVATTEST_CONTENT_TEXT = "assessmentPassFailedText";
const char* SETTINGS_RESOURCE_PATH = "/system/app/com.ohos.settings/Settings.hap";

DevAttestNotificationPublish::DevAttestNotificationPublish()
{
}

DevAttestNotificationPublish::~DevAttestNotificationPublish()
{
}

void DevAttestNotificationPublish::PublishNotification(void)
{
    int32_t publishable = DEVATTEST_INIT;
    int32_t ret = QueryAttestPublishable(&publishable);
    if (ret != DEVATTEST_SUCCESS) {
        HILOGE("[PublishNotification] QueryAttestPublishable fail");
        return;
    }
    if (publishable == DEVATTEST_SUCCESS) {
        HILOGE("[PublishNotification] No need to publish notifications");
        return;
    }

    ret = PublishNotificationImpl();
    if (ret != DEVATTEST_SUCCESS) {
        HILOGE("[PublishNotification] public notification fail");
        return;
    }
    AttestPublishComplete();
    HILOGI("[PublishNotification] publish notification success");
    return;
}

int32_t DevAttestNotificationPublish::PublishNotificationImpl(void)
{
    int32_t uid = 0;
    std::string contentTitle;
    std::string contentText;
    if (GetDevattestBundleUid(&uid) != DEVATTEST_SUCCESS) {
        HILOGE("[PublishNotificationImpl] failed to get uid");
        return DEVATTEST_FAIL;
    }

    if (GetDevattestContent(contentTitle, contentText) != DEVATTEST_SUCCESS) {
        HILOGE("[PublishNotificationImpl] failed to get Content");
        return DEVATTEST_FAIL;
    }

    auto normalContent = std::make_shared<Notification::NotificationNormalContent>();
    if (normalContent == nullptr) {
        HILOGE("[PublishNotificationImpl] normalContent is null");
        return DEVATTEST_FAIL;
    }
    normalContent->SetTitle(contentTitle);
    normalContent->SetText(contentText);
    auto content = std::make_shared<Notification::NotificationContent>(normalContent);
    if (content == nullptr) {
        HILOGE("[PublishNotificationImpl] content is null");
        return DEVATTEST_FAIL;
    }
    Notification::NotificationRequest request;
    request.SetNotificationId(DEVATTEST_PUBLISH_NOTIFICATION_ID);
    request.SetCreatorUid(uid);
    request.SetContent(content);
    request.SetSlotType(Notification::NotificationConstant::OTHER);
    int32_t result = Notification::NotificationHelper::PublishNotification(request);
    if (result != DEVATTEST_SUCCESS) {
        HILOGE("[PublishNotificationImpl] publish result:%{public}d", result);
        return result;
    }
    return DEVATTEST_SUCCESS;
}

int32_t DevAttestNotificationPublish::GetDevattestBundleUid(int32_t* uid)
{
    sptr<ISystemAbilityManager> systemAbilityManager =
        SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        HILOGE("[GetDevattestBundleUid] get systemAbilityManager failed");
        return DEVATTEST_FAIL;
    }

    sptr<IRemoteObject> remoteObject = systemAbilityManager->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (remoteObject == nullptr) {
        HILOGE("[GetDevattestBundleUid] get remoteObject failed");
        return DEVATTEST_FAIL;
    }

    sptr<AppExecFwk::IBundleMgr> bundleMgr = iface_cast<AppExecFwk::IBundleMgr>(remoteObject);
    if (bundleMgr == nullptr) {
        HILOGE("[GetDevattestBundleUid] bundleMgr remoteObject failed");
        return DEVATTEST_FAIL;
    }
    *uid = bundleMgr->GetUidByBundleName(std::string(DEVATTEST_PUBLISH_BUNDLE), DEVATTEST_PUBLISH_USERID);
    return DEVATTEST_SUCCESS;
}

std::shared_ptr<Global::Resource::ResConfig> DevAttestNotificationPublish::GetDevattestResConfig(void)
{
    std::shared_ptr<Global::Resource::ResConfig> pResConfig(Global::Resource::CreateResConfig());

    char locale[LOCALE_SIZE] = {0};
    char language[LOCALE_ITEM_SIZE] = {0};
    char script[LOCALE_ITEM_SIZE] = {0};
    char region[LOCALE_ITEM_SIZE] = {0};
    int32_t ret = GetParameter(PERSIST_GLOBAL_LOCALE_LABEL, DEFAULT_LOCALE_LABEL, locale, LOCALE_SIZE);
    if (ret < DEVATTEST_SUCCESS) {
        HILOGE("[GetDevattestResConfig] failed to get parameter. ret:%{public}d", ret);
        return nullptr;
    }

    ret = sscanf_s(locale, "%" LETTER_PATTERN "-%" LETTER_PATTERN "-%" LETTER_PATTERN,
        language, LOCALE_ITEM_SIZE,
        script, LOCALE_ITEM_SIZE,
        region, LOCALE_ITEM_SIZE);
    if (ret != PARAM_THREE) {
        HILOGE("[GetDevattestResConfig] failed to split locale locale:%{public}s", locale);
        return nullptr;
    }

    Global::Resource::RState state = pResConfig->SetLocaleInfo(language, script, region);
    if (state != Global::Resource::RState::SUCCESS) {
        HILOGE("[GetDevattestResConfig] failed to SetLocaleInfo state:%{public}d", state);
        return nullptr;
    }
    return pResConfig;
}

int32_t DevAttestNotificationPublish::GetDevattestContent(std::string &title, std::string &text)
{
    std::shared_ptr<Global::Resource::ResourceManager> pResMgr(Global::Resource::CreateResourceManager());
    if (pResMgr == nullptr) {
        HILOGE("[GetDevattestContent] get resourceManager failed");
        return DEVATTEST_FAIL;
    }

    if (!pResMgr->AddResource(SETTINGS_RESOURCE_PATH)) {
        HILOGE("[GetDevattestContent] failed to AddResource");
        return DEVATTEST_FAIL;
    }

    std::shared_ptr<Global::Resource::ResConfig> pResConfig = GetDevattestResConfig();
    Global::Resource::RState state = pResMgr->UpdateResConfig(*pResConfig);
    if (state != Global::Resource::RState::SUCCESS) {
        HILOGE("[GetDevattestContent] failed to UpdateResConfig state:%{public}d", state);
        return DEVATTEST_FAIL;
    }

    state = pResMgr->GetStringByName(DEVATTEST_CONTENT_TITLE, title);
    if (state != Global::Resource::RState::SUCCESS) {
        HILOGE("[GetDevattestContent] failed to get title form resource state:%{public}d", state);
        return DEVATTEST_FAIL;
    }

    state = pResMgr->GetStringByName(DEVATTEST_CONTENT_TEXT, text);
    if (state != Global::Resource::RState::SUCCESS) {
        HILOGE("[GetDevattestContent] failed to get text form resource state:%{public}d", state);
        return DEVATTEST_FAIL;
    }
    return DEVATTEST_SUCCESS;
}
} // DevAttest
} // OHOS

