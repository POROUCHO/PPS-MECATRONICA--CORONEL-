using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.IO;
using System.Reflection;

using System.Threading;
using System.Net;
using System.Net.Sockets;

namespace APPDOSIFICADORA
{
    /// <summary>
    /// Lógica de interacción para MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        String linea_com;
        Boolean UDPC = false;
        Int32 B, C;



        private void Window_Initialized(object sender, EventArgs e)
        {
            string pathToData = AppDomain.CurrentDomain.BaseDirectory;
            #if DEBUG
                    pathToData = pathToData.Remove(pathToData.Length - 11);
            #endif
            
            pathToData = System.IO.Path.Combine(pathToData, "Recursos\\BN\\ConfIP.txt");
            
            StreamReader leer = new StreamReader(pathToData);
            linea_com = leer.ReadLine();

            Label_ip.Content = linea_com;

            Thread thdUDPServer = new Thread(new ThreadStart(serverThread)); /// abro hilo para Servidor UDP
            thdUDPServer.Start();

        }

        public void serverThread() //SERVER
        {
            UdpClient udpServer = new UdpClient(8080);

            while (UDPC == false)
            {
                IPEndPoint RemoteIpEndPoint = new IPEndPoint(IPAddress.Any, 8080);
                Byte[] receiveBytes = udpServer.Receive(ref RemoteIpEndPoint);
                string returnData = Encoding.ASCII.GetString(receiveBytes);

                B = returnData.IndexOf("B");
                C = returnData.IndexOf("C");
                
                this.Dispatcher.Invoke(() => Nivel_1.Content = returnData.Substring(0,B));
                this.Dispatcher.Invoke(() => Nivel_2.Content = returnData.Substring(B+1,C-B-1));

            }
            udpServer.Close();
        }

        private void enviar_data (String e)
        {
            UdpClient udpClient = new UdpClient();
            udpClient.Connect(linea_com, 8080);
            Byte[] senddata = Encoding.ASCII.GetBytes(e);
            udpClient.Send(senddata, senddata.Length);
        }

        // Botones ------------------------------------------------

        private void subir_Click(object sender, RoutedEventArgs e)
        {
            enviar_data("subir");
        }

        private void bajar_Click(object sender, RoutedEventArgs e)
        {
            enviar_data("bajar");
        }

        private void detener_Click(object sender, RoutedEventArgs e)
        {
            enviar_data("stop");
        }

        private void comenzar_Click(object sender, RoutedEventArgs e)
        {
            enviar_data("play" + volumen_a.Text + "B" + volumen_b.Text);
        }

        private void Min(object sender, RoutedEventArgs e)
        {
            this.WindowState = WindowState.Minimized;
        }

        private void Close(object sender, RoutedEventArgs e)
        {
            UDPC = true;
            Close();
        }


    }
}
