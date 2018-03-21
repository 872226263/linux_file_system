using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.Windows.Input;

namespace linux_file_system
{
    public partial class Form1 : Form
    {
        public SSH ssh;
        public Form1()
        {
            InitializeComponent();
        }

        public void flush_dir()
        {
            text_path.Text = ssh.si.scppath;
            List<string[]> list = loadDir(ssh.si.scppath);

            this.listView1.Items.Clear();
            int i = 0;
            foreach (string[] s in list)
            {
                listView1.Items.Add(new ListViewItem(s[8]));
                if (s[8].Last() == '/')
                    listView1.Items[i].SubItems.Add("");
                else
                    listView1.Items[i].SubItems.Add(s[4]);
                listView1.Items[i].SubItems.Add(s[5] + "-" + s[6] + "-" + s[7]);
                listView1.Items[i].SubItems.Add(s[2]);
                listView1.Items[i].SubItems.Add(s[0]);
                i++;
            }
        }

        public Form1(SSH ssh)
        {
            InitializeComponent();
            this.ssh = ssh;
            List<string[]> list = loadDir("/home/");
            int i = 0;
            foreach (string[] s in list)
            {
                listView1.Items.Add(new ListViewItem(s[8]));
                if(s[8].Last()=='/')
                    listView1.Items[i].SubItems.Add("");
                else
                    listView1.Items[i].SubItems.Add(s[4]);
                listView1.Items[i].SubItems.Add(s[5]+"-"+s[6]+"-"+s[7]);
                listView1.Items[i].SubItems.Add(s[2]);
                listView1.Items[i].SubItems.Add(s[0]);
                i++;
            }
        }

        public List<string[]> loadDir(string dirpath)
        {
            IntPtr alldir = new IntPtr(); int alldir_len = new int();
            ssh.si.scppath = dirpath;
            SSH.ssh_exec_ls(ssh.si, ref alldir, ref alldir_len);
            string dir = Marshal.PtrToStringAnsi(alldir);
            string[] dirArray = dir.Split(new char[] { '\n' });
            SSH.freeP(ref alldir);
            List<string[]> list = new List<string[]>();
            for (int i = 1; i < dirArray.Length; i++)
            {
                string[] s1 = System.Text.RegularExpressions.Regex.Split(dirArray[i], "\\s+");
                if (s1.Length >= 9)
                    list.Add(new string[] { s1[0], s1[1], s1[2], s1[3], s1[4], s1[5], s1[6], s1[7], s1[8] });
            }
            text_path.Text = dirpath;
            return list;
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            
        }

        private void text_path_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter)     //enter
            {
                List<string[]> list = loadDir(text_path.Text);
                this.listView1.Items.Clear();
                int i = 0;
                foreach (string[] s in list)
                {
                    listView1.Items.Add(new ListViewItem(s[8]));
                    if (s[8].Last() == '/')
                        listView1.Items[i].SubItems.Add("");
                    else
                        listView1.Items[i].SubItems.Add(s[4]);
                    listView1.Items[i].SubItems.Add(s[5] + "-" + s[6] + "-" + s[7]);
                    listView1.Items[i].SubItems.Add(s[2]);
                    listView1.Items[i].SubItems.Add(s[0]);
                    i++;
                }
                //listView1.ColumnClick += new ColumnClickEventHandler(ListViewHelper.ListView_ColumnClick);
            }
        }

        String open_file;
        private void openToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (open_file.Last() == '/')
            {
                ssh.si.scppath+= open_file;
                flush_dir();
            }
        }

        private void contextMenuStrip1_Opening(object sender, CancelEventArgs e)
        {
            ListViewItem lvi =  ((ListView)(sender as ContextMenuStrip).SourceControl).FocusedItem;
            open_file = lvi.Text;
            if (lvi.Text == "./")
            {
                contextMenuStrip1.Visible = false;
                openToolStripMenuItem.Visible = false;
                downloadToolStripMenuItem.Visible = false;
            }
            else if (lvi.Text.Last() == '/')
            {
                openToolStripMenuItem.Visible = true;
                downloadToolStripMenuItem.Visible=false;
            }
            else
            {
                this.openToolStripMenuItem.Visible = false;
                this.downloadToolStripMenuItem.Visible = true;
            }
        }


        private void button1_Click(object sender, EventArgs e)
        {
            OpenFileDialog dialog = new OpenFileDialog();
            dialog.Multiselect = false;//该值确定是否可以选择多个文件
            dialog.Title = "请选择文件夹";
            dialog.Filter = "所有文件(*.*)|*.*";
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                ssh.si.loclfile = dialog.FileName; 
            }
            SSH.scp_write(ssh.si);
        }

        private void downloadToolStripMenuItem_Click(object sender, EventArgs e)
        {
            FolderBrowserDialog folder = new FolderBrowserDialog();
            folder.Description = "选择文件存放目录";
            if (folder.ShowDialog() == DialogResult.OK)
            {

                ssh.si.loclfile = folder.SelectedPath+"\\";
            }
            string temp = ssh.si.scppath;
            ssh.si.scppath = ssh.si.scppath+"/"+ open_file;
            SSH.ssh_exec_read(ssh.si);
            ssh.si.scppath = temp;
        }
    }
}
