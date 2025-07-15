//
// Copyright (c) 2008-2020 the Urho3D project.
// Copyright (c) 2020-2021 LucKey Productions.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

import java.time.Duration

plugins {
    id("com.android.application")
    kotlin("android")
    kotlin("android.extensions")
    dry("android")
}

android {
    ndkVersion = ndkSideBySideVersion
    compileSdkVersion(29)
    defaultConfig {
        minSdkVersion(18)
        targetSdkVersion(29)
        applicationId = "games.luckey.dry.launcher"
        versionCode = 1
        versionName = project.version.toString()
        testInstrumentationRunner = "android.support.test.runner.AndroidJUnitRunner"
        externalNativeBuild {
            cmake {
                arguments.apply {
                    System.getenv("ANDROID_CCACHE")?.let { add("-DANDROID_CCACHE=$it") }
                    add("-DGRADLE_BUILD_DIR=${findProject(":android:dry-lib")?.buildDir}")
                    addAll(
                        listOf(
                            "DRY_LIB_TYPE",
                            // TODO: "DRY_PACKAGING",
                            "DRY_ANGELSCRIPT",
                            "DRY_LUA"
                        )
                            .filter { project.hasProperty(it) }
                            .map { "-D$it=${project.property(it)}" }
                    )
                    // In order to get clean module segregation, only build player/samples here
                    // unless it is explicitly excluded
                    addAll(
                        listOf(
                            "DRY_PLAYER",
                            "DRY_SAMPLES"
                        )
                            .map { "-D$it=${project.findProperty(it) ?: "1"}" }
                    )
                }
            }
        }
        splits {
            abi {
                isEnable = project.hasProperty("ANDROID_ABI")
                reset()
                include(
                    *(project.findProperty("ANDROID_ABI") as String? ?: "")
                        .split(',')
                        .toTypedArray()
                )
            }
        }
    }
    buildTypes {
        named("release") {
            isMinifyEnabled = false
            proguardFiles(getDefaultProguardFile("proguard-android.txt"), "proguard-rules.pro")
        }
    }
    externalNativeBuild {
        cmake {
            setVersion(cmakeVersion)
            setPath(project.file("CMakeLists.txt"))
        }
    }
}

dependencies {
    implementation(fileTree(mapOf("dir" to "libs", "include" to listOf("*.jar"))))
    implementation(project(":android:dry-lib"))
    implementation(kotlin("stdlib-jdk8", embeddedKotlinVersion))
    testImplementation("junit:junit:4.12")
    androidTestImplementation("androidx.test:runner:1.2.0")
    androidTestImplementation("androidx.test.espresso:espresso-core:3.2.0")
}

// Ensure IDE "gradle sync" evaluate the dry-lib module first
evaluationDependsOn(":android:dry-lib")

afterEvaluate {
    tasks {
        "clean" {
            doLast {
                android.externalNativeBuild.cmake.path?.touch()
            }
        }
    }
    android.buildTypes.forEach {
        val config = it.name.capitalize()
        tasks {
            "externalNativeBuild$config" {
                mustRunAfter(":android:dry-lib:externalNativeBuild$config")
                if (System.getenv("CI") != null) {
                    @Suppress("UnstableApiUsage")
                    timeout.set(Duration.ofMinutes(15))
                }
            }
        }
    }
}
