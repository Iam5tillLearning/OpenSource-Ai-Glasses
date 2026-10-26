plugins {
    id("com.android.application")
}

android {
    namespace = "com.osaig.sdk.ble.demo"
    compileSdk = 36

    defaultConfig {
        applicationId = "com.osaig.sdk.ble.demo"
        minSdk = 23
        targetSdk = 36
        versionCode = 1
        versionName = "1.0"
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
}
