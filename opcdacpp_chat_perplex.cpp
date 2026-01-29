#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 28251)

#include <windows.h>
#include <ole2.h>
#include <opcda.h>
#include <iostream>
#include <io.h>
#include <fcntl.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "uuid.lib")

// OPC DA 2.0 GUID
const IID IID_IOPCServer   = {0x39c13a4d, 0x011e, 0x11d0, {0x96, 0x75, 0x00, 0x20, 0xaf, 0xd8, 0xad, 0xb3}};
const IID IID_IOPCItemMgt  = {0x1c3f3e10, 0x14f8, 0x11d0, {0x8e, 0xe1, 0x00, 0xa0, 0xc9, 0x0f, 0x4c, 0xb0}};
const IID IID_IOPCSyncIO   = {0x39c13a50, 0x011e, 0x11d0, {0x96, 0x75, 0x00, 0x20, 0xaf, 0xd8, 0xad, 0xb3}};

int main() {
    HRESULT hr;
    
    // Консоль XP
    setlocale(LC_ALL, "Russian");
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stderr), _O_U16TEXT);

    std::wcout << L"=== OPC DA ТЕСТ v2.0 ===" << std::endl;
    std::wcout << L"=========================" << std::endl;

    // 1. CLSID
    CLSID clsidOPC;
    hr = CLSIDFromProgID(L"opcserversim.Instance.1", &clsidOPC);
    if (FAILED(hr)) {
        std::wcout << L"❌ CLSID не найден: 0x" << std::hex << hr << std::endl;
        return 1;
    }
    std::wcout << L"✓ CLSID OK" << std::endl;

    // 2. COM
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) return hr;

    hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_CONNECT,
        RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    if (FAILED(hr)) {
        CoUninitialize();
        return hr;
    }
    std::wcout << L"✓ COM OK" << std::endl;

    // 3. OPC Server
    IOPCServer* pServer = NULL;
    hr = CoCreateInstance(clsidOPC, NULL, CLSCTX_ALL, IID_IOPCServer, (void**)&pServer);
    if (FAILED(hr)) {
        std::wcout << L"❌ CoCreateInstance: 0x" << std::hex << hr << std::endl;
        std::wcout << L"DCOM: dcomcnfg -> Everyone права" << std::endl;
        CoUninitialize();
        return hr;
    }
    std::wcout << L"✓ OPC Server создан!" << std::endl;

    // 4. Группа (ВАША проблема 0x800706f4 здесь)
    DWORD groupHandle = 0;
    IOPCItemMgt* pItemMgt = NULL;
    DWORD revisedRate = 0;
    
    hr = pServer->AddGroup(L"TestGroup", TRUE, 1000, 1, NULL, NULL, LOCALE_USER_DEFAULT,
                          &groupHandle, &revisedRate, IID_IOPCItemMgt, (LPUNKNOWN*)&pItemMgt);
    std::wcout << L"AddGroup: 0x" << std::hex << hr << std::endl;

    if (FAILED(hr)) {
        std::wcout << L"❌ AddGroup провал! РЕШЕНИЕ:" << std::endl;
        std::wcout << L"1. Запустите opcserversim.exe ОТ АДМИНА" << std::endl;
        std::wcout << L"2. services.msc -> OPC Server Simulator -> Start" << std::endl;
        pServer->Release();
        CoUninitialize();
        return hr;
    }

    std::wcout << L"✓ Группа OK! Handle=" << std::dec << groupHandle << L" Rate=" << revisedRate << std::endl;

    // 5. Тег
    OPCITEMDEF itemDef = {};
    itemDef.szItemID = L"Random.Int1";
    itemDef.bActive = TRUE;
    itemDef.hClient = 1;
    itemDef.vtRequestedDataType = VT_I4;

    OPCITEMRESULT* pResult = NULL;
    HRESULT* pError = NULL;
    hr = pItemMgt->AddItems(1, &itemDef, &pResult, &pError);
    std::wcout << L"AddItems: 0x" << std::hex << hr << std::endl;

    if (SUCCEEDED(hr) && pResult) {
        std::wcout << L"✓ Тег OK! hServer=" << std::dec << pResult[0].hServer << std::endl;

        // 6. Чтение
        IOPCSyncIO* pSyncIO = NULL;
        hr = pItemMgt->QueryInterface(IID_IOPCSyncIO, (void**)&pSyncIO);
        if (SUCCEEDED(hr)) {
            OPCITEMSTATE* pState = NULL;
            HRESULT readError = pSyncIO->Read(OPC_DS_DEVICE, 1, &pResult[0].hServer, &pState, &pError);
            std::wcout << L"Read: 0x" << std::hex << readError << std::endl;
            
            if (SUCCEEDED(readError) && pState) {
                std::wcout << L"✅ УСПЕХ! Random.Int1 = " << std::dec << pState[0].vDataValue.lVal << std::endl;
                CoTaskMemFree(pState);
            }
            pSyncIO->Release();
        }
        CoTaskMemFree(pResult);
        CoTaskMemFree(pError);
    }

    // Cleanup
    pItemMgt->Release();
    pServer->Release();
    CoUninitialize();
    
    std::wcout << L"\n✓ ТЕСТ ЗАВЕРШЕН!" << std::endl;
    std::wcout << L"Нажмите Enter..." << std::endl;
    std::cin.get();
    return 0;
}
результат выполнения : === OPC DA ТЕСТ v2.0 ===
=========================
✓ CLSID OK
✓ COM OK
✓ OPC Server создан!
AddGroup: 0x80004002
❌ AddGroup провал! РЕШЕНИЕ:
1. Запустите opcserversim.exe ОТ АДМИНА
2. services.msc -> OPC Server Simulator -> Start
