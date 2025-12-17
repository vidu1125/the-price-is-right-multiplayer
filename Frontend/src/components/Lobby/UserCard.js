// UserCard.js
import "./UserCard.css";

// SỬA ĐƯỜNG DẪN ẢNH:
// Nó nằm trong thư mục 'bg' của thư mục 'public'.
const DEFAULT_AVATAR_URL = "/bg/default-mushroom.jpg"; 

export default function UserCard() {
  const handleAvatarClick = () => {
    console.log("Open profile modal (later)");
  };

  return (
    <div className="user-card">
      <div className="avatar" onClick={handleAvatarClick}>
        {/* SỬ DỤNG THẺ ẢNH */}
        <img src={DEFAULT_AVATAR_URL} alt="Avatar" />
      </div>
      <div className="user-name">Player Name</div>
    </div>
  );
}