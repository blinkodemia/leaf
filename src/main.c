// vim:foldmethod=marker

// {{{ include

#include "include/lualib.h"
#include "include/lauxlib.h"

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

// }}}
// {{{ define

#define VERSION "1.3.2"
#define LUA_MAIN "/usr/lib/sdx6/leaf/main.lua"
#define LUA_MAIN_LOCAL "/.local/share/sdx6/leaf/main.lua"

// }}}
// {{{ LICENSE

//  ,------------------------------------------------,
// | Permission is hereby granted, free of charge, to |
// | any person obtaining a copy of this software and |
// | associated documentation files (the "Software"), |
// | to deal in the Software without restriction,     |
// | including without limitaion the rights to use,   |
// | copy, modify, merge, publish, distribute,        |
// | sublicense, and/or sell copies of the Software,  |
// | and to permit persons to whom the Software is    |
// | furnished to do so, subject to the following     |
// | contidions:                                      |
// |                                                  |
// | The above copyright notice and this permission   |
// | notice shall be included in all copies or        |
// | substantial portions of the Software.            |
// |                                                  |
// | THE SOFTWARE IS PROIVDED "AS IS", WITHOUT        |
// | WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,        |
// | INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF   |
// | MERCHANTABILITY, FITNESS FOR A PARTICULAR        |
// | PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL   |
// | THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR   |
// | ANY CLAIM, DAMAGES, OR OTHER LIABILITY, WHETHER  |
// | IN AN ACTION OF CONTRACT, TORT, OR OTHERWISE,    |
// | ARISING FROM, OUT OF OR IN CONNECTION WITH THE   |
// | SOFTWARE OR THE USE OR OTHER DEALINGS IN THE     |
// | SOFTWARE.                                        |
//  '-_   _------------------------------------------'
//      V         ,----------------------------------,
//   /\ _ /\     / The Software IS NOT INTENDED to    |
// =( > w < )=_ <  run on OSX, any BSD, or Windows.   |
//   )  v  (_/ | \ Tested only on Linux, FOR Linux!!! |
//  ( _\|/_ )_/   '----------------------------------'

// }}}
// {{{ lua functions

static int getdays(lua_State* lua)
{
  struct sysinfo linuxinfo;
  if(sysinfo(&linuxinfo) != 0) perror("linuxinfo");

  lua_pushinteger(lua, linuxinfo.uptime / 86400);
  return 1;
}

static int gethours(lua_State* lua)
{
  struct sysinfo linuxinfo;
  if(sysinfo(&linuxinfo) != 0) perror("linuxinfo");

  int days = linuxinfo.uptime / 86400;
  lua_pushinteger(lua, (linuxinfo.uptime / 3600) - (days * 24));
  return 1;
}

static int getmins(lua_State* lua)
{
  struct sysinfo linuxinfo;
  if(sysinfo(&linuxinfo) != 0) perror("linuxinfo");

  int days = linuxinfo.uptime / 86400;
  int hours = (linuxinfo.uptime / 3600) - (days * 24);
  lua_pushinteger(lua, (linuxinfo.uptime / 60) - (days * 1440) - (hours * 60));
  return 1;
}

static int getip(lua_State* lua)
{
  const char* d = luaL_checkstring(lua, -1);

  int n;
  struct ifreq i;
  n = socket(AF_INET, SOCK_DGRAM, 0);
  i.ifr_addr.sa_family = AF_INET;
  strncpy(i.ifr_name, d, IFNAMSIZ - 1);
  ioctl(n, SIOCGIFADDR, &i);
  close(n);
  char* r = inet_ntoa(((struct sockaddr_in*)&i.ifr_addr)->sin_addr);

  if (strcmp(r, "0.0.0.0") == 0) lua_pushnil(lua);
  else lua_pushstring(lua, r);
  return 1;
}

// }}}

// {{{ lua error

void error(lua_State* lua, int type)
{
  if (!type)
  {
    fprintf(stderr, "[FATAL ERROR]: %s\n", lua_tostring(lua, -1));
    lua_pop(lua, -1);
    fprintf(stderr, "\nSomething has probably gone horribly wrong, and the program is no longer able to proceed\n");
    fprintf(stderr, "If this seems like a bug, and not user error, please first ensure you are using the latest version (using %s)\n", VERSION);
    fprintf(stderr, "Then please submit a bug report with the error shown just above\n");
    fprintf(stderr, "\nBetter luck next time <3\n");
  }
  else
  {
    fprintf(stderr, "[FATAL ERROR]: leaf is not installed...\n");
  }
  lua_close(lua);
}

// }}}

int main(int argc, char* argv[])
{
  lua_State* lua = lua_open();
  luaL_openlibs(lua);
  
  // {{{ expose lua functions

  static const struct luaL_Reg lua_functions[] =
  {
    {
      "days",
      getdays,
    },
    {
      "hours",
      gethours,
    },
    {
      "mins",
      getmins,
    },
    {
      "ip",
      getip,
    },
    {
      NULL,
      NULL,
    },
  };
  luaL_register(lua, "get", lua_functions);

  // }}} 
  // {{{ expose lua variables & args
  
  if (argc > 0)
  {
    lua_newtable(lua);
      for (int i = 0; i < argc; i++)
      {
        lua_pushstring(lua, argv[i]);
        lua_rawseti(lua, -2, i + 1);
      }
    lua_setglobal(lua, "arg");
    lua_pushstring(lua, VERSION);
    lua_setglobal(lua, "leafver");
  }

  // }}}
  // {{{ start lua

  /* isnt installed to system? */
  if (!access(LUA_MAIN, F_OK))
  {
    if (luaL_dofile(lua, LUA_MAIN))
    {
      error(lua, 0);
      return 1;
    }
  }
  /* isnt installed to user?? */
  else if (!access(strcat(getenv("HOME"), LUA_MAIN_LOCAL), F_OK))
  {
    if (luaL_dofile(lua, LUA_MAIN_LOCAL))
    {
      error(lua, 0);
      return 1;
    }
  }
  /* isnt installed at all??? */
  else
  {
    error(lua, 1);
    return 1;
  }

  // }}}

  lua_close(lua);
  return 0;
}
