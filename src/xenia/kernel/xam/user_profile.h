/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_KERNEL_XAM_USER_PROFILE_H_
#define XENIA_KERNEL_XAM_USER_PROFILE_H_

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "xenia/base/byte_stream.h"
#include "xenia/kernel/util/xuserdata.h"
#include "xenia/xbox.h"

namespace xe {
namespace kernel {
namespace xam {

constexpr uint32_t kDashboardID = 0xFFFE07D1;

// https://github.com/jogolden/testdev/blob/master/xkelib/xam/_xamext.h#L68
enum class XTileType {
  kAchievement = 0x0,
  kGameIcon  = 0x1,
  kGamerTile  = 0x2,
  kGamerTileSmall = 0x3,
  kLocalGamerTile = 0x4,
  kLocalGamerTileSmall = 0x5,
  kBkgnd = 0x6,
  kAwardedGamerTile = 0x7,
  kAwardedGamerTileSmall = 0x8,
  kGamerTileByImageId = 0x9,
  kPersonalGamerTile = 0xA,
  kPersonalGamerTileSmall = 0xB,
  kGamerTileByKey = 0xC,
  kAvatarGamerTile = 0xD,
  kAvatarGamerTileSmall = 0xE,
  kAvatarFullBody = 0xF
};

// TODO: find filenames of other tile types that are stored in profile
static const std::map<XTileType, wchar_t*> kTileFileNames = {
    {XTileType::kPersonalGamerTile, L"tile_64.png"},
    {XTileType::kPersonalGamerTileSmall, L"tile_32.png"},
    {XTileType::kAvatarGamerTile, L"avtr_64.png"},
    {XTileType::kAvatarGamerTileSmall, L"avtr_32.png"},
};

#pragma pack(push, 4)
struct X_XAMACCOUNTINFO {
  enum AccountReservedFlags {
    kPasswordProtected = 0x10000000,
    kLiveEnabled = 0x20000000,
    kRecovering = 0x40000000,
    kVersionMask = 0x000000FF
  };

  enum AccountUserFlags {
    kPaymentInstrumentCreditCard = 1,

    kCountryMask = 0xFF00,
    kSubscriptionTierMask = 0xF00000,
    kLanguageMask = 0x3E000000,

    kParentalControlEnabled = 0x1000000,
  };

  enum AccountSubscriptionTier {
    kSubscriptionTierSilver = 3,
    kSubscriptionTierGold = 6,
    kSubscriptionTierFamilyGold = 9
  };

  // already exists inside xdbf.h??
  enum AccountLanguage {
    kNoLanguage,
    kEnglish,
    kJapanese,
    kGerman,
    kFrench,
    kSpanish,
    kItalian,
    kKorean,
    kTChinese,
    kPortuguese,
    kSChinese,
    kPolish,
    kRussian,
    kNorwegian = 15
  };

  enum AccountLiveFlags { kAcctRequiresManagement = 1 };

  xe::be<uint32_t> reserved_flags;
  xe::be<uint32_t> live_flags;
  wchar_t gamertag[0x10];
  xe::be<uint64_t> xuid_online;  // 09....
  xe::be<uint32_t> cached_user_flags;
  xe::be<uint32_t> network_id;
  char passcode[4];
  char online_domain[0x14];
  char online_kerberos_realm[0x18];
  char online_key[0x10];
  char passport_membername[0x72];
  char passport_password[0x20];
  char owner_passport_membername[0x72];

  bool IsPasscodeEnabled() {
    return (bool)(reserved_flags & AccountReservedFlags::kPasswordProtected);
  }

  bool IsLiveEnabled() {
    return (bool)(reserved_flags & AccountReservedFlags::kLiveEnabled);
  }

  bool IsRecovering() {
    return (bool)(reserved_flags & AccountReservedFlags::kRecovering);
  }

  bool IsPaymentInstrumentCreditCard() {
    return (bool)(cached_user_flags &
                  AccountUserFlags::kPaymentInstrumentCreditCard);
  }

  bool IsParentalControlled() {
    return (bool)(cached_user_flags &
                  AccountUserFlags::kParentalControlEnabled);
  }

  bool IsXUIDOffline() { return ((xuid_online >> 60) & 0xF) == 0xE; }
  bool IsXUIDOnline() { return ((xuid_online >> 48) & 0xFFFF) == 0x9; }
  bool IsXUIDValid() { return IsXUIDOffline() != IsXUIDOnline(); }
  bool IsTeamXUID() {
    return (xuid_online & 0xFF00000000000140) == 0xFE00000000000100;
  }

  uint32_t GetCountry() { return (cached_user_flags & kCountryMask) >> 8; }

  AccountSubscriptionTier GetSubscriptionTier() {
    return (AccountSubscriptionTier)(
        (cached_user_flags & kSubscriptionTierMask) >> 20);
  }

  AccountLanguage GetLanguage() {
    return (AccountLanguage)((cached_user_flags & kLanguageMask) >> 25);
  }

  std::string GetGamertagString() const;
};
static_assert_size(X_XAMACCOUNTINFO, 0x17C);
#pragma pack(pop)

constexpr uint32_t kMaxSettingSize = 0x03E8;

enum class X_USER_PROFILE_SETTING_SOURCE : uint32_t {
  NOT_SET = 0,
  DEFAULT = 1,
  TITLE = 2,
  UNKNOWN = 3,
};

// Each setting contains 0x18 bytes long header
struct X_USER_PROFILE_SETTING_HEADER {
  xe::be<uint32_t> setting_id;
  xe::be<uint32_t> unknown_1;
  xe::be<uint8_t> setting_type;
  char unknown_2[3];
  xe::be<uint32_t> unknown_3;

  union {
    // Size is used only for types: CONTENT, WSTRING, BINARY
    be<uint32_t> size;
    // Raw values that can be written. They do not need to be serialized.
    be<int32_t> s32;
    be<int64_t> s64;
    be<uint32_t> u32;
    be<double> f64;
    be<float> f32;
  };
};
static_assert_size(X_USER_PROFILE_SETTING_HEADER, 0x18);

struct X_USER_PROFILE_SETTING {
  xe::be<uint32_t> from;
  union {
    xe::be<uint32_t> user_index;
    xe::be<uint64_t> xuid;
  };
  xe::be<uint32_t> setting_id;
  union {
    uint8_t data_bytes[sizeof(X_USER_DATA)];
    X_USER_DATA data;
  };
};
static_assert_size(X_USER_PROFILE_SETTING, 40);

class UserSetting {
 public:
  template <typename T>
  UserSetting(uint32_t setting_id, T data) {
    header_.setting_id = setting_id;

    setting_id_.value = setting_id;
    CreateUserData(setting_id, data);
  }

  static bool is_title_specific(uint32_t setting_id) {
    return (setting_id & 0x3F00) == 0x3F00;
  }

  bool is_title_specific() const {
    return is_title_specific(setting_id_.value);
  }

  const uint32_t GetSettingId() const { return setting_id_.value; }
  const X_USER_PROFILE_SETTING_SOURCE GetSettingSource() const {
    return created_by_;
  }
  const X_USER_PROFILE_SETTING_HEADER* GetSettingHeader() const {
    return &header_;
  }
  UserData* GetSettingData() { return user_data_.get(); }

  void SetNewSettingSource(X_USER_PROFILE_SETTING_SOURCE new_source) {
    created_by_ = new_source;
  }

  void SetNewSettingHeader(X_USER_PROFILE_SETTING_HEADER* header) {
    header_ = *header;
  }

 private:
  void CreateUserData(uint32_t setting_id, uint32_t data) {
    header_.setting_type = static_cast<uint8_t>(X_USER_DATA_TYPE::INT32);
    header_.s64 = data;
    user_data_ = std::make_unique<Uint32UserData>(data);
  }
  void CreateUserData(uint32_t setting_id, int32_t data) {
    header_.setting_type = static_cast<uint8_t>(X_USER_DATA_TYPE::INT32);
    header_.s32 = data;
    user_data_ = std::make_unique<Int32UserData>(data);
  }
  void CreateUserData(uint32_t setting_id, float data) {
    header_.setting_type = static_cast<uint8_t>(X_USER_DATA_TYPE::FLOAT);
    header_.f32 = data;
    user_data_ = std::make_unique<FloatUserData>(data);
  }
  void CreateUserData(uint32_t setting_id, double data) {
    header_.setting_type = static_cast<uint8_t>(X_USER_DATA_TYPE::DOUBLE);
    header_.f64 = data;
    user_data_ = std::make_unique<DoubleUserData>(data);
  }
  void CreateUserData(uint32_t setting_id, int64_t data) {
    header_.setting_type = static_cast<uint8_t>(X_USER_DATA_TYPE::INT64);
    header_.s64 = data;
    user_data_ = std::make_unique<Int64UserData>(data);
  }
  void CreateUserData(uint32_t setting_id, const std::u16string& data) {
    header_.setting_type = static_cast<uint8_t>(X_USER_DATA_TYPE::WSTRING);
    header_.size =
        std::min(kMaxSettingSize, static_cast<uint32_t>((data.size() + 1) * 2));
    user_data_ = std::make_unique<UnicodeUserData>(data);
  }
  void CreateUserData(uint32_t setting_id, const std::vector<uint8_t>& data) {
    header_.setting_type = static_cast<uint8_t>(X_USER_DATA_TYPE::BINARY);
    header_.size =
        std::min(kMaxSettingSize, static_cast<uint32_t>(data.size()));
    user_data_ = std::make_unique<BinaryUserData>(data);
  }

  X_USER_PROFILE_SETTING_SOURCE created_by_ =
      X_USER_PROFILE_SETTING_SOURCE::DEFAULT;

  X_USER_PROFILE_SETTING_HEADER header_ = {};
  UserData::Key setting_id_ = {};
  std::unique_ptr<UserData> user_data_ = nullptr;
};

class UserProfile {
 public:
  UserProfile(uint8_t index);

  uint64_t xuid() const { return xuid_; }
  std::wstring path() const;
  std::string name() const { return name_; }
  uint32_t signin_state() const { return 1; }
  uint32_t type() const { return 1 | 2; /* local | online profile? */ }

  void AddSetting(std::unique_ptr<UserSetting> setting);
  UserSetting* GetSetting(uint32_t setting_id);

  std::map<uint32_t, uint32_t> contexts_;

 private:
  uint64_t xuid_;
  std::wstring profile_path_;
  std::wstring base_path_;
  std::string name_;
  std::vector<std::unique_ptr<UserSetting>> setting_list_;
  std::unordered_map<uint32_t, UserSetting*> settings_;

  void LoadSetting(UserSetting*);
  void SaveSetting(UserSetting*);
};

}  // namespace xam
}  // namespace kernel
}  // namespace xe

#endif  // XENIA_KERNEL_XAM_USER_PROFILE_H_
