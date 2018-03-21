using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using System.Windows.Forms;

[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
public struct SSHINFO
{
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 64)]
    public string username;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
    public string password;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 1024)]
    public string loclfile;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 1024)]
    public string scppath;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
    public string hostaddrstr;
    public Int32 port;
}

public class SSH
{
    public SSHINFO si;
    public void init_login(string username, string password, string hostaddrstr,int port)
    {
        si.username = username;
        si.password = password;
        si.hostaddrstr = hostaddrstr;
        si.port = port;
    }
    [DllImport("libssh_for_C_sharp.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern int scp_write(SSHINFO si);
    [DllImport("libssh_for_C_sharp.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern int login_test(SSHINFO si);
    [DllImport("libssh_for_C_sharp.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern int ssh_exec_ls(SSHINFO si, ref IntPtr alldir, ref int alldir_len);
    [DllImport("libssh_for_C_sharp.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern void ssh_exec_read(SSHINFO si);
    [DllImport("libssh_for_C_sharp.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern void freeP(ref IntPtr Block);
}

namespace linux_file_system
{
    static class Program
    {
        /// <summary>
        /// 应用程序的主入口点。
        /// </summary>
        /// 
        [STAThread]

        static void Main()
        {
            SSH ssh = new SSH();


            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new login(ssh));
            
            Application.Run(new Form1(ssh));
        }
    }
}
