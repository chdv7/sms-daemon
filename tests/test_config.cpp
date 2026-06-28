#include <gtest/gtest.h>

#include <fstream>

#include "../src/Config.h"

namespace chdv::sms_daemon::test {

TEST(ConfigTest, LoadsConfiguredPathsAndKeepsDefaults) {
    const std::string path = "/tmp/sms-daemon-config-test.conf";
    std::ofstream(path) << "# daemon settings\n"
                        << " device = /dev/ttyUSB2 \n"
                        << "job_dir=/tmp/jobs\n"
                        << "sms_dir=/tmp/sms-in\n"
                        << "ussd_dir=/tmp/ussd-in\n"
                        << "log_file=/tmp/sms-daemon-test.log\n"
                        << "sms_hook=/usr/local/bin/sms-hook-a\n"
                        << "sms_hook=/usr/local/bin/sms-hook-b\n"
                        << "debug=true\n";

    SmsDaemonConfig config;
    std::string error;
    ASSERT_TRUE(LoadSmsDaemonConfig(path, config, error)) << error;
    EXPECT_EQ(config.device, "/dev/ttyUSB2");
    EXPECT_EQ(config.jobDir, "/tmp/jobs");
    EXPECT_EQ(config.smsDir, "/tmp/sms-in");
    EXPECT_EQ(config.ussdDir, "/tmp/ussd-in");
    EXPECT_EQ(config.logFile, "/tmp/sms-daemon-test.log");
    ASSERT_EQ(config.smsHooks.size(), 2u);
    EXPECT_EQ(config.smsHooks[0], "/usr/local/bin/sms-hook-a");
    EXPECT_EQ(config.smsHooks[1], "/usr/local/bin/sms-hook-b");
    EXPECT_TRUE(config.debug);
}

TEST(ConfigTest, MissingOptionalConfigKeepsDefaults) {
    SmsDaemonConfig config;
    std::string error;
    ASSERT_TRUE(LoadSmsDaemonConfig("/tmp/sms-daemon-missing-config.conf", config, error, false)) << error;
    EXPECT_EQ(config.device, DEVICE);
    EXPECT_EQ(config.jobDir, OUT_SMS_DIR);
    EXPECT_EQ(config.smsDir, IN_SMS_XML_DIR);
    EXPECT_EQ(config.ussdDir, IN_SMS_XML_DIR);
    EXPECT_EQ(config.logFile, SMS_LOG_FILE);
    EXPECT_TRUE(config.smsHooks.empty());
    EXPECT_FALSE(config.debug);
}

TEST(ConfigTest, RejectsUnknownSetting) {
    const std::string path = "/tmp/sms-daemon-bad-config.conf";
    std::ofstream(path) << "unknown=value\n";

    SmsDaemonConfig config;
    std::string error;
    EXPECT_FALSE(LoadSmsDaemonConfig(path, config, error));
    EXPECT_NE(error.find("unknown setting"), std::string::npos);
}

TEST(ConfigTest, RejectsInvalidDebugValue) {
    const std::string path = "/tmp/sms-daemon-bad-debug-config.conf";
    std::ofstream(path) << "debug=maybe\n";

    SmsDaemonConfig config;
    std::string error;
    EXPECT_FALSE(LoadSmsDaemonConfig(path, config, error));
    EXPECT_NE(error.find("invalid boolean value for debug"), std::string::npos);
}

TEST(ConfigTest, RejectsEmptySmsHook) {
    const std::string path = "/tmp/sms-daemon-empty-hook-config.conf";
    std::ofstream(path) << "sms_hook=\n";

    SmsDaemonConfig config;
    std::string error;
    EXPECT_FALSE(LoadSmsDaemonConfig(path, config, error));
    EXPECT_NE(error.find("sms_hook must not be empty"), std::string::npos);
}

} // namespace chdv::sms_daemon::test
