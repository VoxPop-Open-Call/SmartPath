<html>
  <body>
    <?php
    // Get key values for database connection
    $hostname = getenv("HOSTNAME");
    $dbname = getenv("dbnameSecure");
    $username = getenv("usernameSecure");
    $password = getenv("passwordSecure");
    $port = getenv("portSecure");
    
    // GET variables
    $latitude = $_GET["Latitude"];
    $longitude = $_GET["Longitude"];
    $altitude = $_GET["Altitude"];
    $velocity = $_GET["Velocity"];
    $date = $_GET["Date"];
    $time = $_GET["UTCTime"];
    $UPLYID = $_GET["UPLYID"];
            
    // Create connection and insert into table
    $conn = new mysqli($hostname, $username, $password, $dbname, $port);
    if ($conn->connect_error) 
      die("Connection failed: " . $conn->connect_error);

    $stmt = $conn->prepare("INSERT INTO GPS (Latitude, Longitude, Altitude, Velocity, UTCTime, Date, UPLYID) VALUES (?, ?, ?, ?, ?, ?, ?);");
    $stmt->bind_param("ddddssd", $latitude, $longitude, $altitude, $velocity, $time, $date, $UPLYID);
    
    $query = $stmt->execute();

    // Check for erros
    if($query === TRUE)
      echo "Change made successfully";
    else
      echo "An error ocurred: ". $conn->error;
        
    // Close the connection
    $stmt->close();
    $conn->close();
    ?> 
  </body>
</html>