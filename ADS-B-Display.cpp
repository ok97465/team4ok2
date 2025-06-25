//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include <tchar.h>
#include "AirportProvider.h"
#include "RouteProvider.h"

//---------------------------------------------------------------------------
USEFORM("AreaDialog.cpp", AreaConfirm);
USEFORM("DisplayGUI.cpp", Form1);
//---------------------------------------------------------------------------
static FILE* pCout = NULL;
static void SetStdOutToNewConsole(void);
//---------------------------------------------------------------------------
static void SetStdOutToNewConsole(void)
{
    // Allocate a console for this app
    AllocConsole();
    //AttachConsole(ATTACH_PARENT_PROCESS);
	freopen_s(&pCout, "CONOUT$", "w", stdout);
}

//---------------------------------------------------------------------------
int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
	#if 0
    // AirportProvider를 std::unique_ptr<IAviationProvider<AirportInfo>>로 사용
	std::unique_ptr<IAviationProvider<AirportInfo>> airportProvider = std::make_unique<AirportProvider>();
	airportProvider->DownloadAndParseAviationDataAsync(
        [](const std::vector<AirportInfo>& batch, bool finished) {
            if (!batch.empty()) {
				printf("[AIRPORT BATCH] size=%zu\n", batch.size());
                for (const auto& a : batch) {
					printf("%s, %s, %s, %s, %s, %s, %s, %s, %s\n",
                        a.GetCode().c_str(),
                        a.GetName().c_str(),
                        a.GetICAO().c_str(),
                        a.GetIATA().c_str(),
                        a.GetLocation().c_str(),
                        a.GetCountryISO2().c_str(),
                        a.GetLatitude().c_str(),
                        a.GetLongitude().c_str(),
                        a.GetAltitudeFeet().c_str());
                }
            }
            if (finished) {
                printf("[AIRPORT PARSING FINISHED]\n");
			}
        }
    );

    // RouteProvider를 std::unique_ptr<IAviationProvider<RouteInfo>>로 사용
	std::unique_ptr<IAviationProvider<RouteInfo>> routeProvider = std::make_unique<RouteProvider>();
	routeProvider->DownloadAndParseAviationDataAsync(
        [](const std::vector<RouteInfo>& batch, bool finished) {
            if (!batch.empty()) {
                printf("[ROUTE BATCH] size=%zu\n", batch.size());
                for (const auto& r : batch) {
                    printf("%s, %s, %s, %s, %s\n",
                        r.GetCallsign().c_str(),
                        r.GetCode().c_str(),
                        r.GetNumber().c_str(),
                        r.GetAirlineCode().c_str(),
                        r.GetAirportCodes().c_str());
                }
            }
            if (finished) {
                printf("[ROUTE PARSING FINISHED]\n");
			}
        }
    );
#endif
	try
	{
        SetStdOutToNewConsole();
		Application->Initialize();
		Application->MainFormOnTaskBar = true;
		Application->CreateForm(__classid(TForm1), &Form1);
		Application->CreateForm(__classid(TAreaConfirm), &AreaConfirm);
		Application->Run();
	   if (pCout)
		{
		 fclose(pCout);
		 FreeConsole();
	    }
	}
	catch (Exception &exception)
	{
		Application->ShowException(&exception);
	}
	catch (...)
	{
		try
		{
			throw Exception("");
		}
		catch (Exception &exception)
		{
			Application->ShowException(&exception);
		}
	}
	return 0;
}
//---------------------------------------------------------------------------
