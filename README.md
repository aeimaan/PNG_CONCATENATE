<h1>PNG_CONCATENATE</h1>

<p><strong>PNG_CONCATENATE</strong> is a C-based program that concatenates multiple PNG images into a single file. It takes input PNG file paths as command-line arguments and outputs a merged image.</p>

<h2>Features</h2>
<ul>
  <li>Concatenates multiple PNG files into one.</li>
  <li>Utilizes the <code>libpng</code> library for image processing.</li>
  <li>Command-line interface for input and output file paths.</li>
</ul>

<h2>Getting Started</h2>
<ol>
  <li>Clone the repository:</li>
  <pre><code>git clone https://github.com/aeimaan/PNG_CONCATENATE.git</code></pre>
  <li>Compile the program using the provided Makefile:</li>
  <pre><code>make</code></pre>
  <li>Run the program with input PNG files:</li>
  <pre><code>./concat_png input1.png input2.png output.png</code></pre>
</ol>

<h2>Prerequisites</h2>
<ul>
  <li>C compiler (e.g., GCC)</li>
  <li><code>libpng</code> installed</li>
</ul>
