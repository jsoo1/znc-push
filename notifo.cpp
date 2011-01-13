/**
 * ZNC Notifo Module
 *
 * Allows the user to enter a Notifo user and API token, and sends
 * channel highlights and personal messages to Notifo.
 *
 * Copyright (c) 2011 John Reese
 * Licensed under the MIT license
 */

#define REQUIRESSL

#include "znc.h"
#include "Chan.h"
#include "User.h"
#include "Modules.h"

#if (!defined(VERSION_MAJOR) || !defined(VERSION_MINOR) || (VERSION_MAJOR == 0 && VERSION_MINOR < 72))
#error This module needs ZNC 0.072 or newer.
#endif

#define DEBUG_HOST 0
#define DEBUG_LOGGING 0

class CNotifoMod : public CModule
{
	protected:

		// Too lazy to add CString("\r\n\") everywhere
		CString crlf;

		// Host and URL to send messages to
		CString notifo_host;
		CString notifo_url;

		// User agent to use
		CString user_agent;

		// Recipient account's username and API secret
		CString notifo_username;
		CString notifo_secret;

	public:

		MODCONSTRUCTOR(CNotifoMod) {
			crlf = "\r\n";

#if DEBUG_HOST
			notifo_host = "notifo.leetcode.net";
			notifo_url = "/index.php";
#else
			notifo_host = "api.notifo.com";
			notifo_url = "/v1/send_notification";
#endif

			user_agent = "ZNC To Notifo";

			notifo_username = "";
			notifo_secret = "";
		}
		virtual ~CNotifoMod() {}

	protected:

		/**
		 * Shorthand for encoding a string for a URL.
		 *
		 * @param str String to be encoded
		 * @return Encoded string
		 */
		CString urlencode(const CString& str)
		{
			return str.Escape_n(CString::EASCII, CString::EURL);
		}

		/**
		 * Send a message to the currently-configured Notifo account.
		 * Requires (and assumes) that the user has already configured their
		 * username and API secret using the 'set' command.
		 *
		 * @param message Message to be sent to the user
		 */
		void send_message(const CString& message)
		{
			// BASIC auth style
			CString auth = notifo_username + CString(":") + notifo_secret;

			// POST body parameters for the request
			CString post = "to=" + urlencode(notifo_username);
			post += "&msg=" + urlencode(message);
			post += "&label=" + urlencode(CString("ZNC"));
			post += "&title=" + urlencode(CString("New Message"));
			post += "&uri=" + urlencode(CString("http://notifo.leetcode.net/"));

			// Request headers and POST body
			CString request = "POST " + notifo_url + " HTTP/1.1" + crlf;
			request += "Host: " + notifo_host + crlf;
			request += "Content-Type: application/x-www-form-urlencoded" + crlf;
			request += "Content-Length: " + CString(post.length()) + crlf;
			request += "User-Agent: " + user_agent + crlf;
			request += "Authorization: Basic " + auth.Base64Encode_n() + crlf;
			request += crlf;
			request += post + crlf;

			// Create the socket connection, write to it, and add it to the queue
			CSocket *sock = new CSocket(this);
			sock->Connect(notifo_host, 443, true);
			sock->Write(request);
			sock->Close(Csock::CLT_AFTERWRITE);
			AddSocket(sock);

#if DEBUG_LOGGING
			// Log the HTTP request
			FILE *fh = fopen("/tmp/notifo.log", "a");
			fputs(request.c_str(), fh);
			fclose(fh);
#endif
		}

	protected:

		bool OnLoad(const CString& args, CString& message)
		{
			notifo_username = GetNV("notifo_username");
			notifo_secret = GetNV("notifo_secret");

			return true;
		}

		/**
		 * Handle direct commands to the *notifo virtual user.
		 *
		 * @param command Command string
		 */
		void OnModCommand(const CString& command)
		{
			VCString tokens;
			int token_count = command.Split(" ", tokens, false);

			CString action = tokens[0].AsLower();

			// SET command
			if (action == "set")
			{
				if (token_count != 3)
				{
					PutModule("Usage: set <option> <value>");
					return;
				}

				CString option = tokens[1].AsLower();
				CString value = tokens[2];

				if (option == "username")
				{
					SetNV("notifo_username", value);
					notifo_username = value;
				}
				else if (option == "secret")
				{
					SetNV("notifo_secret", value);
					notifo_secret = value;
				}
				else
				{
					PutModule("Error: invalid option name");
					return;
				}

				PutModule("Done");
			}
			// GET command
			else if (action == "get")
			{
				if (token_count != 2)
				{
					PutModule("Usage: get <option>");
					return;
				}

				CString option = tokens[1].AsLower();

				if (option == "username")
				{
					PutModule(GetNV("notifo_username"));
				}
				else if (option == "secret")
				{
					PutModule(GetNV("notifo_secret"));
				}
				else
				{
					PutModule("Error: invalid option name");
					return;
				}
			}
			// SEND command
			else if (action == "send")
			{
				CString message = command.Token(1, true, " ", true);
				send_message(message);
			}
			else
			{
				PutModule("Error: invalid command");
			}
		}
};

MODULEDEFS(CNotifoMod, "Send highlights and personal messages to a Notifo account")