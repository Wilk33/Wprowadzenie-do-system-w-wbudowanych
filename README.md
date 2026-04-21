# STM32 SIL3 Master-Slave Demo

Projekt demonstracyjny architektury Master-Slave przeznaczonej do systemów o podwyższonych wymaganiach bezpieczeństwa (SIL3).

Celem obecnego etapu projektu jest wykonanie prostej operacji sterowania diodą LED na podstawie polecenia użytkownika wysłanego z komputera PC przez UART. Operacja może zostać wykonana przez układ Slave wyłącznie po uzyskaniu zgody od układu Master.

Projekt ma charakter demonstracyjny i służy do opracowania architektury komunikacji, autoryzacji operacji oraz mechanizmów testowania i walidacji.

---

# Architektura systemu

System składa się z trzech elementów:

PC -> UART -> STM32 Slave -> UART -> STM32 Master

PC  
wysyła polecenia użytkownika przez terminal UART.

STM32 Slave  
odbiera polecenia użytkownika, przesyła żądanie autoryzacji do Mastera oraz wykonuje operację dopiero po otrzymaniu poprawnej zgody.

STM32 Master  
nadzoruje wykonywanie operacji i decyduje czy dana operacja może zostać wykonana.

---

# Aktualna funkcjonalność
System realizuje wieloetapowy proces przetwarzania danych, rozpoczynający się od rygorystycznej analizy składniowej komend przychodzących z terminala. 
Po pomyślnej walidacji instrukcja trafia do jednostki nadzorczej, która decyduje o dopuszczeniu operacji do fizycznej realizacji przez moduł wykonawczy. 
Całość procesu zamyka pętla zwrotna, która w przypadku odmowy autoryzacji przesyła do użytkownika precyzyjny komunikat o wystąpieniu błędu.

Przykładowe komendy:

L1 -> odpowiada zasileniu jednej diody LED.  
L4 -> dopowiada zasileniu czterech diod LED.

Slave wykonuje następujące kroki:

1. Odbiera komendę z PC.
2. Sprawdza poprawność składni.
3. Wysyła żądanie autoryzacji do Mastera.
4. Oczekuje na odpowiedź Mastera.
5. W przypadku zgody wykonuje operację ustawienia koloru LED.
6. Zwraca wynik operacji do PC.

Jeżeli zgoda nie zostanie udzielona lub nastąpi błąd komunikacji, operacja nie zostaje wykonana.

---

# Wymagania bezpieczeństwa (etap demonstracyjny)

Slave nie może wykonać operacji bez zgody Mastera.

Każda zgoda Mastera dotyczy konkretnego żądania.

Błędy transmisji są wykrywane przy pomocy CRC.

Brak odpowiedzi Mastera w określonym czasie powoduje anulowanie operacji.

---

# Struktura repozytorium

docs  
Dokumentacja projektu: specyfikacja protokołu, automaty stanów, plan testów.

common  
Kod współdzielony przez Master i Slave.

common/uart_port  
Warstwa sprzętowa UART.

common/safety_protocol  
Protokół komunikacji Master-Slave.

master_fw  
Firmware dla mikrokontrolera STM32 pracującego jako Master.

slave_fw  
Firmware dla mikrokontrolera STM32 pracującego jako Slave.

tests  
Scenariusze testowe oraz testy integracyjne.

tools  
Skrypty pomocnicze dla deweloperów.

---

# Terminal PC

Do komunikacji z układem Slave używany jest standardowy terminal UART.

W projekcie używany jest program PuTTY.

Konfiguracja przykładowa:

Connection type: Serial  
Serial line: COMx  
Speed: 115200  
Data bits: 8  
Stop bits: 1  
Parity: None  
Flow control: None  

Po połączeniu użytkownik może wysyłać komendy sterujące LED.

---

# Scenariusze testowe

Poprawna komenda LED  
Slave uzyskuje zgodę Mastera i ustawia diodę.

Niepoprawna komenda  
Slave zwraca błąd i nie wykonuje operacji.

Brak Mastera  
Slave zgłasza błąd timeout.

Błędna ramka komunikacyjna  
Operacja jest odrzucana.

---

# Zespół projektowy

Project Manager M. Skipor:
zarządzanie projektem, wymagania systemowe, plan testów.

Programista 1 T. Kandziora:
architektura systemu, implementacja Master, protokół komunikacji.

Programista 2 J. Krawczyk:
implementacja Slave, obsługa UART PC, sterowanie LED.

Test / V&V G. Rak:
testy integracyjne, scenariusze testowe oraz walidacja działania systemu.

---

# Status projektu

Etap 1  
Komunikacja UART PC -> Slave

Etap 2  
Protokół komunikacji Slave <-> Master

Etap 3  
Autoryzacja operacji przez Master

Etap 4  
Sterowanie LED

Etap 5  
Testy integracyjne
