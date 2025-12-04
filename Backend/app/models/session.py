from app.models import db

class AccountSession(db.Model):
    __tablename__ = "sessions"

    account_id = db.Column(db.Integer, db.ForeignKey("accounts.id", ondelete="CASCADE"), primary_key=True)
    session_id = db.Column(db.String(36))
    connected = db.Column(db.Boolean)
    updated_at = db.Column(db.DateTime)

    account = db.relationship("Account", back_populates="session")

    def to_dict(self):
        return {
            "account_id": self.account_id,
            "session_id": self.session_id,
            "connected": self.connected,
            "updated_at": self.updated_at.isoformat() if self.updated_at else None,
        }