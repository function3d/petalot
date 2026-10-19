import java.util.Properties

// Signing credentials live outside git (see keystore/README.md).
// keystore.properties is ignored; the template shows the expected keys.
val keystorePropsFile = rootProject.file("keystore.properties")
val keystoreProps = Properties().apply {
    if (keystorePropsFile.exists()) keystorePropsFile.inputStream().use { load(it) }
}
val hasReleaseKey = keystoreProps.getProperty("storeFile") != null

// AGP 9 ships built-in Kotlin support, so no separate Kotlin plugin is needed.
plugins {
    id("com.android.application")
}

android {
    namespace = "com.function3d.petalot"
    compileSdk = 34

    defaultConfig {
        applicationId = "com.function3d.petalot"
        minSdk = 24
        targetSdk = 34
        versionCode = 10800
    versionName = "1.8"
    }

    // Toolchain 17 keeps the build portable; Gradle/IDE run on whatever JDK they like.
    kotlin {
        jvmToolchain(17)
    }

    signingConfigs {
        if (hasReleaseKey) {
            create("release") {
                storeFile = rootProject.file(keystoreProps.getProperty("storeFile"))
                storePassword = keystoreProps.getProperty("storePassword")
                keyAlias = keystoreProps.getProperty("keyAlias")
                keyPassword = keystoreProps.getProperty("keyPassword")
            }
        }
    }

    buildTypes {
        release {
            // Signed only when keystore.properties exists, so a fresh clone
            // can still build debug.
            if (hasReleaseKey) signingConfig = signingConfigs.getByName("release")
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
}
