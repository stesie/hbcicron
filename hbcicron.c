/*
 * Copyright (c) 2005 by Martin Preuss <martin@libchipcard.de>
 * Copyright (c) 2008 by Stefan Siegl <stesie@brokenpipe.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * For more information on the GPL, please go to:
 * http://www.gnu.org/copyleft/gpl.html
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <aqbanking/banking.h>
#include <gwenhywfar/cgui.h>
#include <aqbanking/jobgettransactions.h>

int
main (int argc, char **argv)
{
  AB_BANKING *ab;
  int rv;
  AB_ACCOUNT *a;
  GWEN_GUI *gui;

  if (argc != 3
      || strcmp (argv[1], "--help") == 0
      || strcmp (argv[1], "-h") == 0) {
    fprintf (stderr, "Usage: hbcicron ACCOUNT PINFILE\n"
	     "List HBCI transactions and current balance of ACCOUNT.  Read the"
	     "pincode to use from PINFILE.\n\n");
    return 0;
  }

  const char *accountId = argv[1];
  const char *pinFile = argv[2];

  gui = GWEN_Gui_CGui_new ();
  GWEN_Gui_SetGui (gui);
  GWEN_Gui_CGui_SetCharSet (gui, "ISO-8859-15");

  ab = AB_Banking_new ("hbcicron", 0, 0);

  GWEN_DB_NODE *dbPins = GWEN_DB_Group_new ("pins");
  if (GWEN_DB_ReadFile (dbPins, pinFile, GWEN_DB_FLAGS_DEFAULT
			| GWEN_PATH_FLAGS_CREATE_GROUP, 0, 20000))
    {
      fprintf (stderr, "Error reading pinfile \"%s\"", pinFile);
      return 2;
    }

  GWEN_Gui_CGui_SetPasswordDb (gui, dbPins, 1);

  if ((rv = AB_Banking_Init (ab)))
    {
      fprintf (stderr, "Error on init (%d)\n", rv);
      return 2;
    }
  fprintf (stderr, "AqBanking successfully initialized.\n");

  if ((rv = AB_Banking_OnlineInit (ab)))
    {
      fprintf (stderr, "Error on init of online modules (%d)\n", rv);
      return 2;
    }

  if ((a = AB_Banking_FindAccount (ab, "aqhbci", "de", "*", accountId)))
    {
      AB_JOB_LIST2 *jl;
      AB_JOB *j;
      AB_IMEXPORTER_CONTEXT *ctx;

      /* create a job which retrieves transaction statements. */
      j = AB_JobGetTransactions_new (a);

      /* This function checks whether the given job is available with the
         backend/provider to which the account involved is assigned. */
      rv = AB_Job_CheckAvailability (j, 0);
      if (rv)
	{
	  fprintf (stderr, "Job is not available (%d)\n", rv);
	  return 2;
	}

      jl = AB_Job_List2_new ();
      AB_Job_List2_PushBack (jl, j);

      ctx = AB_ImExporterContext_new ();

      rv = AB_Banking_ExecuteJobs (ab, jl, ctx, 0);
      if (rv)
	{
	  fprintf (stderr, "Error on executeQueue (%d)\n", rv);
	  return 2;
	}
      else
	{
	  AB_IMEXPORTER_ACCOUNTINFO *ai;

	  ai = AB_ImExporterContext_GetFirstAccountInfo (ctx);
	  while (ai)
	    {
	      const AB_TRANSACTION *t;

	      t = AB_ImExporterAccountInfo_GetFirstTransaction (ai);
	      while (t)
		{
		  const AB_VALUE *v;

		  v = AB_Transaction_GetValue (t);
		  if (v)
		    {
		      const GWEN_STRINGLIST *sl;
		      const char *purpose;

		      sl = AB_Transaction_GetPurpose (t);
		      if (sl)
			purpose = GWEN_StringList_FirstString (sl);
		      else
			purpose = "";

		      fprintf (stderr, " %-32s (%.2lf %s)\n",
			       purpose,
			       AB_Value_GetValueAsDouble (v),
			       AB_Value_GetCurrency (v));
		    }
		  t = AB_ImExporterAccountInfo_GetNextTransaction (ai);
		}
	      ai = AB_ImExporterContext_GetNextAccountInfo (ctx);
	    }
	}
      AB_Job_free (j);
    }
  else
    {
      fprintf (stderr, "No account found.\n");
    }

  rv = AB_Banking_OnlineFini (ab);
  if (rv)
    {
      fprintf (stderr, "ERROR: Error on deinit online modules (%d)\n", rv);
      return 3;
    }

  rv = AB_Banking_Fini (ab);
  if (rv)
    {
      fprintf (stderr, "ERROR: Error on deinit (%d)\n", rv);
      return 3;
    }
  AB_Banking_free (ab);

  return 0;
}
