using System.Runtime.InteropServices;
using System.Text;

namespace Refvalue.Samples.SvcHost
{
    [ComVisible(true)]
    [Guid("47D7093F-69E2-D17D-422D-49BE836EF3A5")]
    [ClassInterface(ClassInterfaceType.None)]
    public class TestService : ISvcHostify
    {
        private int _running = 0;

        /// <summary>
        /// The service entry point.
        /// </summary>
        /// <param name="args">The input arguments</param>
        public void Run(string[] args)
        {
            Console.WriteLine("A Svchost run from CSharp.");
            Console.WriteLine("All outputs to Console.out will be redirected to the logging file that you configured.");
            Console.WriteLine("Input arguments:");
            
            foreach (var item in args)
            {
                Console.WriteLine(item);
            }

            var fileName = "output_csharp.txt";
            var text = "It's good to write text to your own file for logging.";

            using (var writer = new StreamWriter(fileName, false, Encoding.UTF8))
            {
                writer.Write(text);
            }

            Interlocked.Exchange(ref _running, 1);

            for (int i = 0; Interlocked.CompareExchange(ref _running, 1, 1) == 1; i++)
            {
                Console.WriteLine($"Hello service counter: {i}");
                Thread.Sleep(100);
            }

            Console.WriteLine("Service has stopped from CSharp.");
        }

        /// <summary>
        /// This function will be called by the SvcHostify routine in another thread and
        /// you can, for example, send a signal to your 'Run' routine to stop gracefully.
        /// </summary>
        public void OnStop()
        {
            Console.WriteLine("A stop signal received.");
            Console.WriteLine("Requesting a stop.");

            Interlocked.Exchange(ref _running, 0);
        }
    }
}
