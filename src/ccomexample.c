#define UNICODE
#define _UNICODE
#include <windows.h>
#include <ShObjIdl.h>

int CCOMExample() {
    // requires having called CoInitialize()

    IFileOpenDialog* pFileOpen;
    HRESULT hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
            &IID_IFileOpenDialog, (void**) &pFileOpen);
    if (FAILED(hr)) {
        printf("Failed to make file open dialog, %ld", hr);
        return 0;
    }

    hr = pFileOpen->lpVtbl->Show(pFileOpen, NULL);
    if (FAILED(hr)) {
        printf("Failed to show file open dialog, %ld", hr);
        return 0;
    }

    IShellItem* pItem;
    hr = pFileOpen->lpVtbl->GetResult(pFileOpen, &pItem);
    if (FAILED(hr)) {
        printf("Failed to get file opened, %ld", hr);
        return 0;
    }
    
    PWSTR pszFilePath;
    hr = pItem->lpVtbl->GetDisplayName(pItem, SIGDN_FILESYSPATH, &pszFilePath);
    if (FAILED(hr)) {
        printf("Failed to get file path display name, %ld", hr);
        return 0;
    }

    MessageBoxW(NULL, pszFilePath, L"File Path", MB_OK);

    CoTaskMemFree(pszFilePath);
    pItem->lpVtbl->Release(pItem);
    pFileOpen->lpVtbl->Release(pFileOpen);

    return 0;
}
