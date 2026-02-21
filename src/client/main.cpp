#include "NetworkManager.h"
#include "controller/AuthController.h"

void printUsage() {
    std::cout << "\n--- Komutlar ---\n";
    std::cout << "login : Login istegi gonderir\n";
    std::cout << "chat  : Test chat mesaji gonderir\n";
    std::cout << "exit  : Programi kapatir\n";
    std::cout << "----------------\n> " << std::flush;
}

int main() {
    std::cout << "--- Uygulama Baslatiliyor (Main Giris) ---" << std::endl;

    try {
        // 1. Network Manager Kurulumu
        NetworkManager netMgr;

        // 2. Controller Kurulumu (NetworkManager icindeki dispatcher'i kullaniyoruz!)
        // DIKKAT: Ayri bir dispatcher yaratmiyoruz.
        lakot::AuthController tAuthController(&netMgr.getDispatcher());
        tAuthController.initialize();

        // 3. Baslatma
        netMgr.start(); // Iceride 127.0.0.1:12345 default degerleri olmali

        std::atomic<bool> running{true};

        // 4. Input Thread
        std::thread inputThread([&]() {
            printUsage();
            std::string line;
            while (running && std::getline(std::cin, line)) {
                if (line == "exit") {
                    running = false;
                    break;
                } else if (line == "login") {

                    // GUVENLIK KONTROLU: Session var mi?
                    auto tSession = netMgr.getClient().getSession();
                    if (!tSession) {
                        std::cout << "[Hata] Henuz baglanilmadi!" << std::endl;
                        continue;
                    }

                    connection::Message msg;

                    static int requestIdProvider = 0;

                    int tRequestId = requestIdProvider++;

                    msg.mutable_request()->mutable_header()->set_request_id(tRequestId);

                    auto* req = msg.mutable_request();
                    req->mutable_login_request()->set_username("TestUser");
                    req->mutable_login_request()->set_password("123456");

                    // Session uzerinden istek gonderiyoruz
                    tSession->sendRequest(msg, [](const connection::Message& response) {
                        // BURASI MAIN THREAD (NetworkClient::update sayesinde)
                        if (response.has_response()) {
                            std::cout << "[Callback] Cevap ID: " << response.response().header().request_id() << std::endl;
                        }
                    }, tRequestId);

                    std::cout << "[Sistem] Login istegi gonderildi.\n> " << std::flush;

                } else if (line == "chat") {
                    auto tSession = netMgr.getClient().getSession();
                    if (tSession) {
                        connection::Message msg;
                        // Chat payload doldur...
                        tSession->send(msg);
                        std::cout << "[Sistem] Chat mesaji gonderildi.\n> " << std::flush;
                    } else {
                        std::cout << "[Hata] Baglanti yok.\n> " << std::flush;
                    }
                }
            }
        });

        // 5. ANA DONGU (Game Loop / Main Thread)
        std::cout << "[Sistem] Ana dongu calisiyor (Network Update)..." << std::endl;

        while (running) {
            // ESKI: MainThreadExecutor::getInstance().process();  <-- BU YANLIS

            // YENI: NetworkManager uzerinden update
            netMgr.update();

            // CPU'yu bogmamak icin minik sleep (Oyun motorunda VSync bunu yapar)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        std::cout << "[Sistem] Kapatiliyor..." << std::endl;
        netMgr.stop();

        if (inputThread.joinable()) {
            inputThread.join();
        }

    } catch (const std::exception& e) {
        std::cerr << "[KRITIK HATA] " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
