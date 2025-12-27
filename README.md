# Project Name: A_WCG

**Full Title:** Atomic Web Component Generator

### 🚀 Tagline

_From Browser to Blueprint: Transforming Web Interfaces into Native Unreal Engine Assets._

### 📝 Project Description

**A_WCG** is a specialized parsing and conversion tool designed to bridge the gap between modern web development and Unreal Engine 5.

It functions as the ingestion engine for the **P_MWCS** (Modular Widget Creation System). Instead of manually recreating UI layouts in Unreal, A_WCG takes a standard website (HTML/CSS/JS) and programmatically deconstructs it. It automatically generates the necessary **C++ Classes** to handle logic and variables, alongside structured **JSON schemas** that define the visual design. This allows developers to build complex UI components using familiar web technologies (like React or plain HTML/CSS) and instantly deploy them as native, performant widgets within Unreal Engine.

### ⚙️ How It Works

A_WCG operates as a translation layer in the pipeline:

1. **Input (The Web Source):**

- Accepts a standalone web page (HTML/CSS/JS).
- Identifies interactive elements (buttons, inputs) and dynamic data fields.

2. **Process (The Generator):**

- **Logic Extraction:** Maps JavaScript variables and functions to native **C++ Class** headers.
- **Design Extraction:** Parses CSS styling and HTML structure into a optimized **JSON** format readable by the engine.

3. **Output (The Integration):**

- Produces plug-and-play assets ready to be consumed by **P_MWCS**.
- Ensures strict separation of concerns: Logic in C++, Visuals in JSON.

### 🔑 Key Features

- **Automated C++ Binding:** Generates header files with ready-to-use `UFUNCTION` and `UPROPERTY` declarations derived from the web code.
- **JSON Layout Schema:** Converts DOM structures into a lightweight JSON format that P_MWCS uses to reconstruct the UI at runtime.
- **P_MWCS Optimized:** Specifically architected to feed data into the Modular Widget Creation System, ensuring 100% compatibility.
- **Stack Agnostic:** Works with raw HTML/CSS or build outputs from frameworks like React, Vue, or Svelte.

---

### 📊 The Workflow

```text
[ Web Development ]       [ A_WCG Tool ]              [ Unreal Engine 5 ]
     (React/HTML)    --->   (Parser)      --->    (P_MWCS Plugin)
          |                     |                         |
          |                     |--> .h (C++ Logic)       |
     Index.html                 |                         |--> Native Widget
                                |--> .json (Design)       |

```
