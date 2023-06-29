<html>
  <body>
    <?php
    // Get key values for database connection
    $hostname = getenv("HOSTNAME");
    $dbname = getenv("dbnameSecure");
    $username = getenv("usernameSecure");
    $password = getenv("passwordSecure");
    $port = getenv("portSecure");
    
    // GET variables (some are always included)
    $latitude = $_GET["Latitude"];
    $longitude = $_GET["Longitude"];
    $altitude = $_GET["Altitude"];
    $date = $_GET["Date"];
    $UPLYID = $_GET["UPLYID"];
    
    if($_GET["OpeningUTCTime"]) {
      $opening = $_GET["OpeningUTCTime"];
      $table = "Openings";    // Got table
    }
    if($_GET["ClosingUTCTime"])
      $closing = $_GET["ClosingUTCTime"];
    if($_GET["OpenedTime"])
      $diff = $_GET["OpenedTime"];
    if($_GET["Velocity"])   {
      $velocity = $_GET["Velocity"];
      $table = "GPS"; // Got table
    }
    if($_GET["UTCTime"])
      $time = $_GET["UTCTime"];
            
    /*
    // Prettify latitude and longitude
    $latitudeTemp = $latitude;
    $longitudeTemp = $longitude;
    $latitude = "";
    $longitude = "";
    
    $degree = false;    // This is to prevent an infinite loop
    
    // Check if latitude is negative
    if($latitudeTemp[0] == '-') {
        $spot = 3;  // Spot to place '°'
    }
    else    {
        $spot = 2;
    }
    
    for($i = 0; $i < strlen($latitudeTemp); $i++)   {
        if($i == $spot && $degree == false) {
            $latitude .= '°';
            $degree = true;
            $i -= 1;
        }
        else    {
            $latitude .= $latitudeTemp[$i];
        }
    }
    
    
    $degree = false;    // This is to prevent an infinite loop
    
    // Check if longitude is negative
    if($longitudeTemp[0] == '-') {
        $spot = 3;  // Spot to place '°'
    }
    else    {
        $spot = 2;
    }
    
    for($i = 0; $i < strlen($longitudeTemp); $i++)   {
        if($i == $spot && $degree == false) {
            $longitude .= '°';
            $degree = true;
            $i -= 1;
        }
        else    {
            $longitude .= $longitudeTemp[$i];
        }
    }
    */
    // Create connection and insert into table
    $conn = new mysqli($hostname, $username, $password, $dbname, $port);
    if ($conn->connect_error) 
      die("Connection failed: " . $conn->connect_error);

    if($table == "Openings")  {
      $stmt = $conn->prepare("INSERT INTO Openings (OpeningUTCTime, ClosingUTCTime, OpenedTime, Latitude, Longitude, Altitude, Date, UPLYID) VALUES (?, ?, ?, ?, ?, ?, ?, ?);");
      $stmt->bind_param("sssdddsd", $opening, $closing, $diff, $latitude, $longitude, $altitude, $date, $UPLYID);
    }
    else  {
      $stmt = $conn->prepare("INSERT INTO GPS (Latitude, Longitude, Altitude, Velocity, UTCTime, Date, UPLYID) VALUES (?, ?, ?, ?, ?, ?, ?);");
      $stmt->bind_param("ddddssd", $latitude, $longitude, $altitude, $velocity, $time, $date, $UPLYID);
    }
    
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