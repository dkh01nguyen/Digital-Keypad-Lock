# Digital Keypad Lock

## Overview
**Digital Keypad Lock** là hệ thống khóa cửa điện tử nhập số được xây dựng trên nền tảng vi điều khiển STM32F103C8T6.  
Dự án tập trung vào ba mục tiêu cốt lõi:
- Đảm bảo **bảo mật cao** thông qua thuật toán xác thực mật khẩu và hiển thị nội dung tối ưu.  
- Triển khai **đa nhiệm hiệu quả** bằng cooperative scheduler và timer interrupt.  
- Đảm bảo **khả năng đáp ứng thời gian thực** cho các thao tác nhập liệu, chuyển trạng thái và hiển thị trực quan đến người dùng.

## Sơ đồ nguyên lý dự án
<img width="4041" height="2841" alt="digital_keypad_lock_schematic" src="https://github.com/user-attachments/assets/89ed94a1-0589-4845-8221-3b82bba56657" />
*Lưu ý:* sản phẩm thực tế không sử dụng **buzzer 12.0V** nên không cần sử dụng relay cho thiết bị này.
---

## Project Architecture
Mã nguồn được tổ chức theo cấu trúc module hóa với hai thư mục chính:  
- `Core/Inc`: chứa các file header định nghĩa interface.  
- `Core/Src`: chứa implementation.
Các thư mục khác của folder mã nguồn (stm32-sourceode) được STM32CubeIDE tạo tự động khi người dùng cấu hình ioc cho dự án và cài đặt các file thư viện, cấu hình mặc định.

### 🔧 System Management Layer
- **main.c / main.h**  
  - Khởi tạo clock và ngoại vi (GPIO, I2C, Timer) qua HAL.  
  - Add task và khai báo dispatch ở vòng lặp chính của chương trình.  

- **scheduler.c / scheduler.h**  
  - Bộ lập lịch cộng tác (cooperative scheduler).  
  - Quản lý danh sách task với chu kỳ riêng. 

- **timer.c / timer.h**  
  - Software timers hỗ trợ timeout detection, delay non-blocking.  
  - Quản lý sự kiện dựa trên thời gian mà không gián đoạn luồng chính.  

- **global.c / global.h**  
  - Quản lý biến toàn cục, buffer dữ liệu, cờ trạng thái.  
  - Điểm giao tiếp dữ liệu giữa các module.  

### ⚙️ Logic and Algorithm Layer
- **state_processing.c / state_processing.h**  
  - Finite State Machine quản lý hành vi hệ thống.  
  - Nhận thao tác từ input, bật cờ (sử dụng output) hoặc chuyển trạng thái theo thiết kế. 

- **input_processing.c / input_processing.h**  
  - Xử lý dữ liệu thô từ input device.  
  - Edge detection, phân tích pattern nhấn phím trên keypad 4x4 và các phím nhấn riêng lẻ.  
  - Sinh event gửi cho state machine.  

- **output_processing.c / output_processing.h**  
  - Điều khiển LCD, solenoid lock, LED, buzzer.  
  - Quản lý hiển thị LCD: căn lề, định dạng chuỗi, hiển thị các thông báo trạng thái,...  

- **kmp.c / kmp.h**  
  - Thuật toán Knuth-Morris-Pratt (KMP) để xác thực mật khẩu.  
  - Hiệu suất O(n+m), tối ưu khi mật khẩu không khớp một phần.
  - Hỗ trợ khả năng xác thực mật khẩu chính xác liên tục (4 ký tự) trong chuỗi nhập (<= 20 ký tự).

### Hardware Interface Layer
- **keypad.c / keypad.h**  
  - Driver cho ma trận phím 4x4.  
  - Quét phím non-blocking, mapping mã phím sang ký tự logic.  

- **input_reading.c / input_reading.h**  
  - Đọc trạng thái nút nhấn rời, cảm biến cửa, mechanical key.  
  - Tích hợp debouncing để loại bỏ nhiễu.  

- **i2c_lcd.c / i2c_lcd.h**  
  - Thư viện giao tiếp LCD 16x2 qua I2C.  
  - API cấp cao: khởi tạo, xóa màn hình, di chuyển con trỏ, hiển thị chuỗi...

---

## Key Features
- Xác thực mật khẩu bằng thuật toán KMP với khả năng bảo mật được nâng cấp.  
- Hỗ trợ đa nhiệm qua cooperative scheduler, không cần RTOS.  
- Giao diện LCD 16x2 trực quan, phản hồi rõ ràng cho thao tác người dùng.  
- Tích hợp cảm biến bảo vệ như door sensor, và cơ chế dự phòng trong - ngoài bằng mechanical key và indoor unlock button (override tất cả trạng thái khóa).    


---

## Author
- **Nguyễn Hảo Khang** – Quản lý repo github và tổng hợp mã nguồn dự án (main, global, state_processing,...).
- **Dương Khôi Nguyên** - Hiện thực module input (input_processing, input_reading, keypad) và trình bày báo cáo dự án (mã nguồn latex trong folder final).
- **Trần Hoàng Bá Huy** - Hiện thực module output (output_processing, chỉnh sửa i2c_lcd), lắp mạch và debug.

  Liên hệ thông qua GitHub Issues hoặc email: khang.nguyen720005@hcmut.edu.vn để được hỗ trợ hoặc đóng góp ý tưởng cho dự án.
