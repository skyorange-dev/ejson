# ejson

[![XERJ Corpus Hub contributor](https://hub.xerj.org/contributors/SkyOrang/badge.svg)](https://hub.xerj.org/contributors/SkyOrang)

Based on the C/C++ standard library, programmers can easily perform common operations such as adding, deleting, querying and modifying various fields in JSON files with excellent performance.

For detailed instructions, please refer to the wiki.

**Usage Guidelines**

The high performance of ejson relies on correct usage.

ejson is not a beginner-friendly, zero-configuration JSON parser like nlohmann. To achieve O(1) time complexity for access operations and near 100% memory efficiency, you must follow our best practices (batch insertion and handle caching). Misuse of the API will result in significant performance degradation. All tests above reflect optimal results leveraging two key features of ejson: inserting large batches of strings at once and handle caching. If your test scenario involves searching from the root each time, the performance difference will be substantial—reflecting the gap between O(1) and O(n) time complexity.

**Characteristics**

- Utilizes interfaces for simplicity and efficiency (based on [], <<, >> operator overloading)

- Excellent performance
  
- Cross-platform (based on C/C++ standard library)

**Performance testing**

The test code is located in the "test" folder of the source code. Those who are interested can try running it on their own computers.

Test environment: i7-12650H, Windows 11, MSVC Release (/O2)

Test content: (Reading and Writing Test)Compare the three core indicators of ejson with nlohmann/json and RapidJSON in a pure memory environment.

Parsing query: Parse a 4KB message and extract deep nodes.

From scratch construction: Dynamically assemble a complex message containing 100 objects.

Memory consumption: The growth of physical memory fragments after extreme high-frequency construction.

EN(enrwtest_main.cpp):

<img width="1427" height="587" alt="image" src="https://github.com/user-attachments/assets/a08a0e6d-b52a-4fae-b1fe-5082047ae5b0" >

CN(cnrwtest_main.cpp):

<img width="1385" height="531" alt="image" src="https://github.com/user-attachments/assets/0fe053cd-dcda-4540-b519-930155191048" />


Test content: (Read-only test)In a pure memory environment, 1 million high-frequency reads of deep nodes were simulated using 4KB messages. ejson uses a loop-
based  extraction handle (to record the absolute physical offset) for O(1) direct access; simdjson reads using its DOM interface after SIMD parsing.

EN(enromain.cpp):

<img width="1382" height="377" alt="image" src="https://github.com/user-attachments/assets/30f80b3d-a648-436c-9b01-2e2e1f02330b" />

CN(cnromain.cpp):

<img width="1436" height="417" alt="image" src="https://github.com/user-attachments/assets/82d7f528-aded-4377-9246-50cbd0839f13" />






