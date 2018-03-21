using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace linux_file_system
{
    public partial class login : Form
    {
        public SSH ssh;
        public login()
        {
            InitializeComponent();
        }

        public login(SSH ssh)
        {
            InitializeComponent();
            this.ssh = ssh;
        }

        private void button1_Click(object sender, EventArgs e)
        {
            string ip = text_ip.Text;
            int port = Convert.ToInt32(text_port.Text == "" ? "0" : text_port.Text);
            string username = text_username.Text;
            string password = text_password.Text;
            ssh.init_login(username, password, ip, port);
            if (SSH.login_test(ssh.si) == 0)
            {
                MessageBox.Show("login success");
                this.Close();
            }

        }
    }
}
