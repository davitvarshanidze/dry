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

import org.gradle.internal.io.NullOutputStream
import org.gradle.internal.os.OperatingSystem
import java.time.Duration

plugins {
    id("com.android.library")
    id("com.jfrog.bintray")
    kotlin("android")
    kotlin("android.extensions")
    `maven-publish`
}

android {
    ndkVersion = ndkSideBySideVersion
    compileSdkVersion(29)
    defaultConfig {
        minSdkVersion(18)
        targetSdkVersion(29)
        versionCode = 1
        versionName = project.version.toString()
        testInstrumentationRunner = "android.support.test.runner.AndroidJUnitRunner"
        externalNativeBuild {
            cmake {
                arguments.apply {
                    System.getenv("ANDROID_CCACHE")?.let { add("-DANDROID_CCACHE=$it") }
                    add("-DGRADLE_BUILD_DIR=$buildDir")
                    // Pass along matching Gradle properties as CMake build options
                    addAll(
                        listOf(
                            "DRY_LIB_TYPE",
                            "DRY_ANGELSCRIPT",
                            "DRY_LUA",
                            "DRY_LUAJIT",
                            "DRY_LUAJIT_AMALG",
                            "DRY_IK",
                            "DRY_NETWORK",
                            "DRY_PHYSICS",
                            "DRY_NAVIGATION",
                            "DRY_2D",
                            "DRY_PCH",
                            "DRY_DATABASE_SQLITE",
                            "DRY_WEBP",
                            "DRY_FILEWATCHER",
                            "DRY_PROFILING",
                            "DRY_LOGGING",
                            "DRY_THREADING"
                        )
                            .filter { project.hasProperty(it) }
                            .map { "-D$it=${project.property(it)}" }
                    )
                    // In order to get clean module segregation, always exclude player/samples from AAR
                    addAll(listOf(
                        "DRY_PLAYER",
                        "DRY_SAMPLES"
                    ).map { "-D$it=0" })
                }
                targets.add("Dry")
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
            setPath(project.file("../../CMakeLists.txt"))

            // Make it explicit as one of the task needs to know the exact path and derived from it
            setBuildStagingDirectory(".cxx")
        }
    }
    sourceSets {
        getByName("main") {
            java.srcDir("../../Source/ThirdParty/SDL/android-project/app/src/main/java")
        }
    }
}

dependencies {
    implementation(fileTree(mapOf("dir" to "libs", "include" to listOf("*.jar"))))
    implementation(kotlin("stdlib-jdk8", embeddedKotlinVersion))
    implementation("com.getkeepsafe.relinker:relinker:1.3.1")
    testImplementation("junit:junit:4.12")
    androidTestImplementation("androidx.test:runner:1.2.0")
    androidTestImplementation("androidx.test.espresso:espresso-core:3.2.0")
}

lateinit var docABI: String

afterEvaluate {
    // Part of the our external native build tree resided in Gradle buildDir
    // When the buildDir is cleaned then we need a way to re-configure that part back
    // It is achieved by ensuring that CMake configuration phase is rerun
    tasks {
        "clean" {
            doLast {
                android.externalNativeBuild.cmake.path?.touch()
            }
        }
    }

    // This is a hack - workaround Android plugin for Gradle not providing way to bundle extra "stuffs"
    android.buildTypes.forEach { buildType ->
        val config = buildType.name.capitalize()
        tasks {
            register<Zip>("zipBuildTree$config") {
                archiveClassifier.set(buildType.name)
                archiveExtension.set("aar")
                dependsOn("zipBuildTreeConfigurer$config", "bundle${config}Aar")
                from(zipTree(getByName("bundle${config}Aar").outputs.files.first()))
            }
            register<Task>("zipBuildTreeConfigurer$config") {
                val externalNativeBuildDir = File(buildDir, "tree/$config")
                doLast {
                    val zipTask = getByName<Zip>("zipBuildTree$config")
                    externalNativeBuildDir.list()?.forEach { abi ->
                        listOf("include", "lib").forEach {
                            zipTask.from(File(externalNativeBuildDir, "$abi/$it")) {
                                into("tree/$config/$abi/$it")
                            }
                        }
                        if (config == "Release") {
                            docABI = abi
                        }
                    }
                }
            }
            if (System.getenv("CI") != null) {
                "externalNativeBuild$config" {
                    @Suppress("UnstableApiUsage")
                    timeout.set(Duration.ofMinutes(25))
                }
            }
        }
    }
}

tasks {
    register<Jar>("sourcesJar") {
        archiveClassifier.set("sources")
        from(android.sourceSets.getByName("main").java.srcDirs)
    }
    register<Exec>("makeDoc") {
        // Ignore the exit status on Windows host system because Doxygen may not return exit status correctly on Windows
        isIgnoreExitValue = OperatingSystem.current().isWindows
        standardOutput = NullOutputStream.INSTANCE
        args("--build", ".", "--target", "doc")
        dependsOn("makeDocConfigurer")
        mustRunAfter("zipBuildTreeRelease")
    }
    register<Zip>("documentationZip") {
        archiveClassifier.set("documentation")
        dependsOn("makeDoc")
    }
    register<Task>("makeDocConfigurer") {
        doLast {
            val buildTree = File(android.externalNativeBuild.cmake.buildStagingDirectory, "cmake/release/$docABI")
            named<Exec>("makeDoc") {
                // This is a hack - expect the first line to contain the path to the CMake executable
                executable = File(buildTree, "build_command.txt").readLines().first().split(":").last().trim()
                workingDir = buildTree
            }
            named<Zip>("documentationZip") {
                from(File(buildTree, "Docs/html")) {
                    into("docs")
                }
            }
        }
    }
}

publishing {
    publications {
        register<MavenPublication>("mavenAndroid") {
            artifactId = "${project.name}-${project.libraryType}"
            if (project.hasProperty("ANDROID_ABI")) {
                artifactId = "$artifactId-${(project.property("ANDROID_ABI") as String).replace(',', '-')}"
            }
            afterEvaluate {
                // Exclude publishing STATIC-debug AAR because its size exceeds 250MB limit allowed by Bintray
                android.buildTypes
                    .map { it.name }
                    .filter { System.getenv("CI") == null || project.libraryType == "SHARED" || it == "release" }
                    .forEach { artifact(tasks["zipBuildTree${it.capitalize()}"]) }
            }
            artifact(tasks["sourcesJar"])
            artifact(tasks["documentationZip"])
            pom {
                @Suppress("UnstableApiUsage")
                inceptionYear.set("2008")
                @Suppress("UnstableApiUsage")
                licenses {
                    license {
                        name.set("MIT License")
                        url.set("https://gitlab.com/luckeyproductions/dry/-/blob/master/LICENSE")
                    }
                }
                @Suppress("UnstableApiUsage")
                developers {
                    developer {
                        name.set("Dry contributors")
                        url.set("https://gitlab.com/luckeyproductions/dry/-/blob/master/CONTRIBUTING.md")
                    }
                }
                @Suppress("UnstableApiUsage")
                scm {
                    url.set("https://gitlab.com/luckeyproductions/Dry.git")
                    connection.set("scm:git:ssh://git@gitlab.com:luckeyproductions/Dry.git")
                    developerConnection.set("scm:git:ssh://git@gitlab.com:luckeyproductions/Dry.git")
                }
                withXml {
                    asNode().apply {
                        appendNode("name", "Dry")
                        appendNode("description", project.description)
                        appendNode("url", "https://dry.luckey.games/")
                    }
                }
            }
        }
    }
}

bintray {
    user = System.getenv("BINTRAY_USER")
    key = System.getenv("BINTRAY_KEY")
    publish = true
    override = true
    setPublications("mavenAndroid")
    pkg.apply {
        repo = "maven"
        name = project.name
        setLicenses("MIT")
        vcsUrl = "https://gitlab.com/luckeyproductions/Dry.git"
        userOrg = "dry"
        setLabels("android", "game-development", "game-engine", "open-source", "dry")
        websiteUrl = "https://dry.luckey.games/"
        issueTrackerUrl = "https://gitlab.com/luckeyproductions/dry/-/issues"
        githubRepo = "luckeyproductions/Dry"
        publicDownloadNumbers = true
        desc = project.description
        version.apply {
            name = project.version.toString()
            desc = "Continuous delivery from Travis-CI."
        }
    }
}

val Project.libraryType: String
    get() = findProperty("DRY_LIB_TYPE") as String? ?: "STATIC"
